#pragma once

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <cstdint>
#include <glad/glad.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#include <stb/stb_image.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <myHelper/shader_s.h>
#include <myHelper/cMesh.h>




// DDS header structure
struct DDSHeader {
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitchOrLinearSize;
    uint32_t depth;
    uint32_t mipMapCount;
    uint32_t reserved1[11];

    struct {
        uint32_t size;
        uint32_t flags;
        uint32_t fourCC;
        uint32_t RGBBitCount;
        uint32_t RBitMask;
        uint32_t GBitMask;
        uint32_t BBitMask;
        uint32_t ABitMask;
    } pixelFormat;

    struct {
        uint32_t caps1;
        uint32_t caps2;
        uint32_t caps3;
        uint32_t caps4;
    } caps;

    uint32_t reserved2;
};

// DX10 header structure
struct DDSHeaderDX10 {
    uint32_t dxgiFormat;
    uint32_t resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t miscFlags2;
};

struct MipmapData {
    uint32_t width;
    uint32_t height;
    std::vector<uint8_t> data;
};

class cModel {
public:
    std::vector<cMesh> meshes;
    std::string directory;
    std::unordered_map<std::string, GLuint> colorTextureCache;
    std::unordered_map<std::string, GLuint> normalTextureCache;
    std::unordered_map<std::string, GLuint> aoTextureCache;


    int textureLoadCount = 0;
    int normalTextureLoadCount = 0;

    cModel(const char* path)
    {
        std::string directoryPath = path;
        directory = directoryPath.substr(0, directoryPath.find_last_of("/") + 1);
        loadModel(path);
    }

    void Draw(Shader& shader)
    {
        for (unsigned int i = 0; i < meshes.size(); i++) {
            shader.setMat4("model", meshes[i].transform);
            meshes[i].draw(shader);
        }
    }

    
    std::vector<MipmapData> readDDS(const std::string& filePath, DDSHeader& header, DDSHeaderDX10& headerDX10)
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open DDS file");
        }

        // Read magic number
        char magic[4];
        file.read(magic, sizeof(magic));
        if (file.gcount() != sizeof(magic) || std::strncmp(magic, "DDS ", sizeof(magic)) != 0) {
            throw std::runtime_error("Not a valid DDS file: " + filePath);
        }

        // Read header
        file.read(reinterpret_cast<char*>(&header), sizeof(DDSHeader));
        if (file.gcount() != sizeof(DDSHeader)) {
            throw std::runtime_error("Failed to read DDS header: " + filePath);
        }

        // Check for DX10 header and read it if present
        if (header.pixelFormat.fourCC == 0x30315844) {
            file.read(reinterpret_cast<char*>(&headerDX10), sizeof(DDSHeaderDX10));
            if (file.gcount() != sizeof(DDSHeaderDX10)) {
                throw std::runtime_error("Failed to read DX10 header: " + filePath);
            }
        }


        std::vector<MipmapData> mipmaps;
        uint32_t width = header.width;
        uint32_t height = header.height;

        for (uint32_t level = 0; level < header.mipMapCount; ++level) {
            size_t dataSize = ((width + 3) / 4) * ((height + 3) / 4) * 16; // BC7 block size is 16 bytes
            mipmaps.push_back({ width, height, std::vector<uint8_t>(dataSize) });

            file.read(reinterpret_cast<char*>(mipmaps[level].data.data()), dataSize);
            if (file.gcount() != dataSize) {
                throw std::runtime_error("Failed to read mipmap level data: " + filePath);
            }

            width = std::max(1U, width / 2);
            height = std::max(1U, height / 2);
        }

        return mipmaps;
    }

    void checkGLError(const std::string& message)
    {
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cerr << "OpenGL error during " << message << ": " << err << std::endl;
        }
    }
    
    GLuint loadDDSTexture(const std::string& path)
    {
        DDSHeader header;
        DDSHeaderDX10 headerDX10 = {};
        auto data = readDDS(path, header, headerDX10);

        GLenum format = 0;

        // Constants
        const uint32_t FOURCC_DXT1 = 0x31545844; // 'DXT1'
        const uint32_t FOURCC_DXT3 = 0x33545844; // 'DXT3'
        const uint32_t FOURCC_DXT5 = 0x35545844; // 'DXT5'
        const uint32_t FOURCC_DX10 = 0x30315844; // 'DX10'
        const uint32_t DXGI_FORMAT_BC7_UNORM = 98;
        const uint32_t DXGI_FORMAT_BC7_UNORM_SRGB = 99;


        switch (header.pixelFormat.fourCC) {
        case FOURCC_DXT1:
            format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            break;
        case FOURCC_DXT3:
            format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            break;
        case FOURCC_DXT5:
            format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            break;
        case FOURCC_DX10:
            if (headerDX10.dxgiFormat == DXGI_FORMAT_BC7_UNORM) {
                format = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
            }
            else if (headerDX10.dxgiFormat == DXGI_FORMAT_BC7_UNORM_SRGB) {
                format = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB;
            }
            else {
                throw std::runtime_error("Unsupported DXGI format in DX10 header");
            }
            break;
        default:
            throw std::runtime_error("Unsupported DDS format");
        }


        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        for (size_t level = 0; level < data.size(); ++level) {
            const auto& mipmap = data[level];
            glCompressedTexImage2D(GL_TEXTURE_2D, level, format, mipmap.width, mipmap.height, 0, mipmap.data.size(), mipmap.data.data());
            checkGLError("texture upload at mipmap level " + std::to_string(level));
        }

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);

        return textureID;
    }

    GLuint loadStandardTexture(const std::string& path)
    {
        int width, height, nrChannels;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

        if (!data) {
            std::cerr << "Failed to load texture: " << path << std::endl;
            return 0; // Return 0 to indicate failure
        }

        
        GLuint textureID;
        GLenum format = GL_RGB; // Default format

        switch (nrChannels) {
        case 1:
            format = GL_RED;
            break;
        case 3:
            format = GL_RGB;
            break;
        case 4:
            format = GL_RGBA;
            break;
        default:
            std::cerr << "Unsupported number of channels: " << nrChannels << std::endl;
            stbi_image_free(data);
            return 0; // Return 0 to indicate failure
        }

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);

        stbi_image_free(data);

        return textureID;
        
    }

    void loadTexture(cgltf_texture* texture, GLuint& textureID, std::unordered_map<std::string, GLuint>& textureCache, int& textureLoadCount)
    {
        cgltf_image* image = texture->image;

        if (image && image->uri) {
            std::string fullPath = directory + '/' + image->uri;

            if (textureCache.find(image->uri) != textureCache.end()) {
                textureID = textureCache[image->uri];
            }
            else {
                textureLoadCount++;
                std::string fileExtension = fullPath.substr(fullPath.find_last_of(".") + 1);
                if (fileExtension == "dds") {
                    textureID = loadDDSTexture(fullPath);
                }
                else {
                    textureID = loadStandardTexture(fullPath);
                }
                textureCache[image->uri] = textureID;
            }
        }
    }

    Material createMaterial(cgltf_primitive* primitive)
    {
        Material newMaterial = Material();
        glm::vec4 color(1.0f);

        if (primitive->material) {
            cgltf_material* material = primitive->material;
            cgltf_pbr_metallic_roughness* pbr = &material->pbr_metallic_roughness;

            if (primitive->material->normal_texture.texture) {
                loadTexture(material->normal_texture.texture, newMaterial.normalTextureID, normalTextureCache, normalTextureLoadCount);
            }
            
            if (pbr) {
                color = glm::vec4(pbr->base_color_factor[0], pbr->base_color_factor[1],
                                  pbr->base_color_factor[2], pbr->base_color_factor[3]);
                newMaterial.baseColor = color;
            }

            if (pbr->base_color_texture.texture) {
                loadTexture(pbr->base_color_texture.texture, newMaterial.colorTextureID, colorTextureCache, textureLoadCount);
            }
        }
        return newMaterial;

    }
    


    void loadModel(const char* path)
    {
        // Parse the GLTF file
        cgltf_options options = {};
        cgltf_data* data = nullptr;
        cgltf_result result = cgltf_parse_file(&options, path, &data);

        if (result != cgltf_result_success) {
            std::cerr << "Failed to parse GLTF file: " << path << std::endl;
            return;
        }

        // Load buffers
        result = cgltf_load_buffers(&options, data, path);
        if (result != cgltf_result_success) {
            std::cerr << "Failed to load buffers for GLTF file: " << path << std::endl;
            cgltf_free(data);
            return;
        }

        // Process Nodes
        for (cgltf_size i = 0; i < data->nodes_count; ++i) {
            if (!data->nodes[i].parent) { // Process root nodes
                processNode(&data->nodes[i]);
            }
        }

        // Free the loaded data
        cgltf_free(data);
    }

    void processNode(cgltf_node* node, const glm::mat4& parentTransform = glm::mat4(1.0f))
    {
        if (!node) {
            std::cerr << "Invalid node pointer" << std::endl;
            return;
        }

        glm::mat4 nodeTransform = parentTransform;

        if (node->has_matrix) {
            glm::mat4 matrix; 
            memcpy(glm::value_ptr(matrix), node->matrix, sizeof(node->matrix));
            nodeTransform = parentTransform * matrix;
        } else {
            // Default translation, rotation, and scale
            glm::vec3 translation(0.0f), scale(1.0f);
            glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);

            // Apply node transformations if present
            if (node->has_translation) {
                translation = glm::make_vec3(node->translation);
            }
            if (node->has_rotation) {
                rotation = glm::make_quat(node->rotation);
            }
            if (node->has_scale) {
                scale = glm::make_vec3(node->scale);
            }

            // Construct the transformation matrix from T, R, S
            glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
            glm::mat4 R = glm::toMat4(rotation);
            glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);

            nodeTransform = parentTransform * T * R * S;
        }

        // Process the mesh attached to the node if it exists
        if (node->mesh) {
            meshes.push_back(processMesh(node->mesh, nodeTransform));
        }

        // Process children nodes recursively
        for (cgltf_size i = 0; i < node->children_count; ++i) {
            processNode(node->children[i], nodeTransform);
        }
    }



    void extractAttributes(cgltf_primitive* primitive, cgltf_accessor*& positions, cgltf_accessor*& normals, cgltf_accessor*& texCoords, cgltf_accessor*& tangents)
    {
        for (int i = 0; i < primitive->attributes_count; i++) {
            switch (primitive->attributes[i].type) {
            case cgltf_attribute_type_position:
                positions = primitive->attributes[i].data;
                break;
            case cgltf_attribute_type_normal:
                normals = primitive->attributes[i].data;
                break;
            case cgltf_attribute_type_texcoord:
                texCoords = primitive->attributes[i].data;
                break;
            case cgltf_attribute_type_tangent:
                tangents = primitive->attributes[i].data;
                break;
            default:
                break;
            }
        }
    }

    float* getBufferData(cgltf_accessor* accessor)
    {
        if (!accessor || !accessor->buffer_view || !accessor->buffer_view->buffer) {
            std::cerr << "Invalid accessor or buffer view" << std::endl;
            return nullptr;
        }
        return reinterpret_cast<float*>(static_cast<char*>(accessor->buffer_view->buffer->data) + accessor->buffer_view->offset + accessor->offset);
    }


    cMesh processMesh(cgltf_mesh* mesh, glm::mat4 transform) {
        std::vector<cPrimitive> primitives;
        GLenum index_type = GL_UNSIGNED_SHORT;

        for (cgltf_size p = 0; p < mesh->primitives_count; ++p) {
            cgltf_primitive* primitive = &mesh->primitives[p];

            cgltf_accessor* positions = nullptr;
            cgltf_accessor* v_normals = nullptr;
            cgltf_accessor* tex_coords = nullptr;
            cgltf_accessor* tangents = nullptr;

            extractAttributes(primitive, positions, v_normals, tex_coords, tangents);

            if (primitive->indices) {
            
                switch (primitive->indices->component_type) {
                    case cgltf_component_type_r_8u:
                        index_type = GL_UNSIGNED_BYTE;
                        break;
                    case cgltf_component_type_r_16u:
                        index_type = GL_UNSIGNED_SHORT;
                        break;
                    case cgltf_component_type_r_32u:
                        index_type = GL_UNSIGNED_INT;
                        break;
                    default:
                        std::cerr << "Unknown index component type" << std::endl;
                        continue;
                }
            }

            float* vertices = getBufferData(positions);
            float* normData = getBufferData(v_normals);
            float* texData = getBufferData(tex_coords);
            float* tangData = getBufferData(tangents);

            if (!vertices || !normData) {
                std::cerr << "Error accessing buffer data for positions or normals" << std::endl;
                continue;
            }
       
            // Assuming indices are present and required for rendering
            cgltf_accessor* indices = primitive->indices;
    

            std::vector<float> interleavedData;
            interleavedData.reserve(positions->count * 11);

            for (size_t i = 0; i < positions->count; ++i) {
                interleavedData.insert(interleavedData.end(), {
                    vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2],
                    normData[i * 3], normData[i * 3 + 1], normData[i * 3 + 2],
                    tangData ? tangData[i * 3] : 0.0f,
                    tangData ? tangData[i * 3 + 1] : 0.0f,
                    tangData ? tangData[i * 3 + 2] : 0.0f,
                    texData ? texData[i * 2] : 0.0f,
                    texData ? texData[i * 2 + 1] : 0.0f
                });
            }

            // Material
            Material newMaterial;
            try {
                newMaterial = createMaterial(primitive);
                // std::cout << "Texture loaded successfully. Texture ID: " << newMaterial.textureID << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "on primitive: " << p << std::endl;
            }

            // Create and bind VAO, VBO, EBO
            GLuint VAO, EBO, VBO;
            glGenVertexArrays(1, &VAO);
            glBindVertexArray(VAO);

            // Interleaved VBO
            glGenBuffers(1, &VBO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(float), interleavedData.data(), GL_STATIC_DRAW);

            const size_t stride = 11 * sizeof(float); // 3 (position) + 3 (normal) + 3 (tangent) + 2 (texcoord) 
            // Positions
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
            glEnableVertexAttribArray(0);
            // Normals
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            // Tangents
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
            glEnableVertexAttribArray(2);
            // Texture Coordinates
            glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)(9 * sizeof(float)));
            glEnableVertexAttribArray(3);


            // Indices
            glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            if (index_type == GL_UNSIGNED_INT)
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->count * sizeof(unsigned int), getBufferData(primitive->indices), GL_STATIC_DRAW);
            else
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->count * sizeof(unsigned short), getBufferData(primitive->indices), GL_STATIC_DRAW);

            // Unbind VAO to avoid accidentally modifying it
            glBindVertexArray(0);

            // Add the new primitive to the primitives vector
            primitives.push_back(cPrimitive(VAO, indices->count, index_type, newMaterial));
        }

        cMesh newMesh(primitives);
        newMesh.transform = transform;
        std::cout << "number of color textures loaded: " << textureLoadCount << std::endl;
        std::cout << "number of normal textures loaded: " << normalTextureLoadCount << std::endl;
        return newMesh;
    }


};
