#include "pch.h"
#include "cModel.h"
#include "texture.h"
#include "MipmapData.h"
#include <algorithm>

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>
#include <thread>
#include <future>
#include <mutex>

struct Timer
{
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    std::chrono::duration<float> duration;
    std::string m_title = "Timer";
    Timer()
    {
        startTimer();
    }
    void setTitle(const char* title)
    {
        m_title = title;
    }
    void startTimer()
    {
        start = std::chrono::high_resolution_clock::now();
    }
    void stopTimer()
    {
        end = std::chrono::high_resolution_clock::now();
        duration = end - start;

        float ms = duration.count() * 1000.0f;

        std::cout << m_title << " took: " << ms << "ms" << std::endl;
    }
};







cModel::cModel(const char* path)
{
    std::string directoryPath = path;
    directory = directoryPath.substr(0, directoryPath.find_last_of("/") + 1);
    loadModel(path);

}

void cModel::Draw(Shader& shader)
{
    for (unsigned int i = 0; i < meshes.size(); i++) 
    {
        shader.setMat4("model", meshes[i].transform);
        meshes[i].draw(shader);
    }
}



void cModel::checkGLError(const std::string& message)
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) 
    {
        std::cerr << "OpenGL error during " << message << ": " << err << std::endl;
    }
}


void cModel::loadTexture(cgltf_texture* texture, Texture& textureObject, std::unordered_map<std::string, Texture>& textureCache, int& textureLoadCount)
{
    cgltf_image* image = texture->image;
    if (image && image->uri)
    {
        std::string fullPath = directory + '/' + image->uri;

        if (textureCache.find(image->uri) != textureCache.end())
        {
            textureObject = textureCache[image->uri];
        }
        else
        {
            textureLoadCount++;
            std::string fileExtension = fullPath.substr(fullPath.find_last_of(".") + 1);
            if (fileExtension == "dds")
            {
               // textureID = loadDDSTexture(fullPath);
                
                textureObject = Texture(fullPath, true);
            }
            else
            {
                //textureObject = loadStandardTexture(fullPath);
                textureObject = Texture(fullPath, false);
            }
            textureCache[image->uri] = textureObject;
        }
    }
    checkGLError("Error");
}

Material cModel::createMaterial(cgltf_primitive* primitive)
{
    Material newMaterial = Material();
    glm::vec4 color(1.0f);

    if (primitive->material)
    {
        cgltf_material* material = primitive->material;
        cgltf_pbr_metallic_roughness* pbr = &material->pbr_metallic_roughness;
        newMaterial.hasColorTexture = false;
        if (primitive->material->normal_texture.texture)
        {
            loadTexture(material->normal_texture.texture, newMaterial.normalTexture, normalTextureCache, normalTextureLoadCount);

        }

        if (pbr)
        {
            color = glm::vec4(pbr->base_color_factor[0], pbr->base_color_factor[1],
                pbr->base_color_factor[2], pbr->base_color_factor[3]);
            newMaterial.baseColor = color;
        }

        if (pbr->base_color_texture.texture)
        {
            newMaterial.hasColorTexture = true;
            loadTexture(pbr->base_color_texture.texture, newMaterial.colorTexture, colorTextureCache, textureLoadCount);
        }
    }
    checkGLError("Error");
    return newMaterial;
}

void cModel::loadModel(const char* path)
{
    // Parse the GLTF file
    cgltf_options options = {};
    cgltf_data* data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, path, &data);

    if (result != cgltf_result_success) 
    {
        std::cerr << "Failed to parse GLTF file: " << path << std::endl;
        return;
    }

    // Load buffers
    result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success) 
    {
        std::cerr << "Failed to load buffers for GLTF file: " << path << std::endl;
        cgltf_free(data);
        return;
    }

    std::vector<std::future<void>> futures;

    // Process Nodes
    for (cgltf_size i = 0; i < data->nodes_count; ++i) 
    {
        if (!data->nodes[i].parent) 
        { // Process root nodes
            processNode(&data->nodes[i]);
        }
    }

    // Free the loaded data
    cgltf_free(data);
}

void cModel::processNode(cgltf_node* node, const glm::mat4& parentTransform)
{
    if (!node) 
    {
        std::cerr << "Invalid node pointer" << std::endl;
        return;
    }

    glm::mat4 nodeTransform = parentTransform;

    if (node->has_matrix) 
    {
        glm::mat4 matrix;
        memcpy(glm::value_ptr(matrix), node->matrix, sizeof(node->matrix));
        nodeTransform = parentTransform * matrix;
    }
    else 
    {
        // Default translation, rotation, and scale
        glm::vec3 translation(0.0f), scale(1.0f);
        glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);

        // Apply node transformations if present
        if (node->has_translation) 
        {
            translation = glm::make_vec3(node->translation);
        }
        if (node->has_rotation) 
        {
            rotation = glm::make_quat(node->rotation);
        }
        if (node->has_scale) 
        {
            scale = glm::make_vec3(node->scale);
        }

        // Construct the transformation matrix from T, R, S
        glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
        glm::mat4 R = glm::toMat4(rotation);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);

        nodeTransform = parentTransform * T * R * S;
    }

    // Process the mesh attached to the node if it exists
    if (node->mesh) 
    {
        meshes.push_back(processMesh(node->mesh, nodeTransform));
    }

    // Process children nodes recursively
    for (cgltf_size i = 0; i < node->children_count; ++i) 
    {
        processNode(node->children[i], nodeTransform);
    }
}

void cModel::extractAttributes(cgltf_primitive* primitive, cgltf_accessor*& positions, cgltf_accessor*& normals, cgltf_accessor*& texCoords, cgltf_accessor*& tangents)
{
    for (int i = 0; i < primitive->attributes_count; i++) 
    {
        switch (primitive->attributes[i].type) 
        {
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

float* cModel::getBufferData(cgltf_accessor* accessor)
{
    if (!accessor || !accessor->buffer_view || !accessor->buffer_view->buffer) 
    {
        std::cerr << "Invalid accessor or buffer view" << std::endl;
        return nullptr;
    }
    return reinterpret_cast<float*>(static_cast<char*>(accessor->buffer_view->buffer->data) + accessor->buffer_view->offset + accessor->offset);
}




cMesh cModel::processMesh(cgltf_mesh* mesh, glm::mat4 transform)
{
    std::vector<cPrimitive> primitives;
    GLenum index_type = GL_UNSIGNED_SHORT;
    
    for (cgltf_size p = 0; p < mesh->primitives_count; ++p) 
    {
        try
        {
            cgltf_primitive* primitive = &mesh->primitives[p];
            cPrimitive newPrimitive = processPrimitive(primitive, index_type);
            primitives.push_back(newPrimitive);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error processing primitive " << p << ": " << e.what() << std::endl;
            continue;
        }
    }


 
    cMesh newMesh(primitives);
    newMesh.transform = transform;
    std::cout << "number of color textures loaded: " << textureLoadCount << std::endl;
    std::cout << "number of normal textures loaded: " << normalTextureLoadCount << std::endl;
    return newMesh;
}

cPrimitive cModel::processPrimitive(cgltf_primitive* primitive, GLenum& index_type)
{
    cgltf_accessor* positions = nullptr;
    cgltf_accessor* v_normals = nullptr;
    cgltf_accessor* tex_coords = nullptr;
    cgltf_accessor* tangents = nullptr;

    extractAttributes(primitive, positions, v_normals, tex_coords, tangents);

    if (primitive->indices)
    {
        switch (primitive->indices->component_type)
        {
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
            throw std::runtime_error("Unknown index component type");
        }
    }

    float* vertices = getBufferData(positions);
    float* normData = getBufferData(v_normals);
    float* texData = getBufferData(tex_coords);
    float* tangData = getBufferData(tangents);

    if (!vertices || !normData)
    {
        std::cerr << "Error accessing buffer data for positions or normals" << std::endl;
        throw std::runtime_error("Error accessing buffer data for positions or normals");
    }

    std::vector<float> interleavedData;
    interleavedData.reserve(positions->count * 11);

    for (size_t i = 0; i < positions->count; ++i)
    {
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

    std::vector<unsigned int> indices;
    if (primitive->indices)
    {
        indices.resize(primitive->indices->count);
        void* bufferData = getBufferData(primitive->indices);
        if (index_type == GL_UNSIGNED_INT)
        {
            std::copy_n(static_cast<unsigned int*>(bufferData), primitive->indices->count, indices.begin());
        }
        else if (index_type == GL_UNSIGNED_SHORT)
        {
            std::vector<unsigned short> shortIndices(primitive->indices->count);
            std::copy_n(static_cast<unsigned short*>(bufferData), primitive->indices->count, shortIndices.begin());
            indices.assign(shortIndices.begin(), shortIndices.end());
        }
        else if (index_type == GL_UNSIGNED_BYTE)
        {
            std::vector<unsigned char> byteIndices(primitive->indices->count);
            std::copy_n(static_cast<unsigned char*>(bufferData), primitive->indices->count, byteIndices.begin());
            indices.assign(byteIndices.begin(), byteIndices.end());
        }
    }

    for (auto& future : m_Futures) {
        future.get();
    }
    m_Futures.clear();

    Material newMaterial;
    try
    {
        newMaterial = createMaterial(primitive);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        throw;
    }

    return cPrimitive(interleavedData, indices, index_type, newMaterial);
}



void cModel::uploadToGpu()
{
    for (auto& mesh : meshes)
    {
        mesh.uploadToGpu();
    }
}
