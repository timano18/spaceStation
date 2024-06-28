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

#define DRACO_TRANSCODER_SUPPORTED
#include <draco/compression/decode.h>
#include <draco/core/decoder_buffer.h>
#include "base64.h"

int totalPrimitives = 0;

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

        std::cout << m_title << " took: " << ms << "ms" << "\n";
    }
};







cModel::cModel(const char* path)
{
    std::string directoryPath = path;
    directory = directoryPath.substr(0, directoryPath.find_last_of("/") + 1);
    loadModel(path);

}

void cModel::Draw(Shader& shader, bool useBatchRendering)
{
    /*
    auto renderFunc = useBatchRendering
        ? [](cMesh& mesh, Shader& shader) { mesh.renderBatch(shader); }
    : [](cMesh& mesh, Shader& shader) { mesh.draw(shader); };
 
    for (auto& mesh : meshes)
    {
        shader.setMat4("model", mesh.transform);
        renderFunc(mesh, shader);
    }
    */
    
    if (useBatchRendering)
    {
        shader.setMat4("model", glm::mat4(1.0));
        renderModelBatch(shader);
    }
    else
    {
        for (auto& mesh : meshes)
        {
            shader.setMat4("model", mesh.transform);
            mesh.draw(shader);
        }
    }
    

}



void cModel::checkGLError(const std::string& message)
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) 
    {
        std::cerr << "OpenGL error during " << message << ": " << err << "\n";
    }
}

bool is_base64_encoded_image(const char* uri)
{
    const char* base64_prefix = "data:image/";
    const char* base64_marker = "base64,";

    if (strncmp(uri, base64_prefix, strlen(base64_prefix)) == 0) {
        const char* base64_position = strstr(uri, base64_marker);
        if (base64_position != NULL) {
            return true;
        }
    }
    return false;
}

std::string urlDecode(const std::string& encoded)
{
    std::ostringstream decoded;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && (i + 2) < encoded.length()) {
            std::istringstream iss(encoded.substr(i + 1, 2));
            int hexValue;
            if (iss >> std::hex >> hexValue) {
                decoded << static_cast<char>(hexValue);
                i += 2;
            }
            else {
                decoded << encoded[i];
            }
        }
        else if (encoded[i] == '+') {
            decoded << ' ';
        }
        else {
            decoded << encoded[i];
        }
    }
    return decoded.str();
}

void cModel::loadTexture(cgltf_texture* texture, std::shared_ptr<Texture>& textureObject)
{
    cgltf_image* image = texture->image;

    if (image->buffer_view)
    {
        textureObject = std::make_shared<Texture>();
        void* bufferData = image->buffer_view->buffer->data;
        size_t bufferOffset = image->buffer_view->offset;
        size_t bufferLength = image->buffer_view->size;

        unsigned char* imageData = static_cast<unsigned char*>(bufferData) + bufferOffset;
        textureObject->loadStandardTextureFromBuffer(imageData, bufferLength);
        textureObject->m_isDDS = false;
    }
    if (image && image->uri)
    {
        std::string path = urlDecode(image->uri);
        if (is_base64_encoded_image(image->uri))
        {
            std::string base64String = path.substr(path.find(',') + 1);
            std::cout << "Image is base64 encoded" << "\n";

            std::string decodedData = base64_decode(base64String);
            textureObject = std::make_shared<Texture>();
            textureObject->loadStandardTextureFromBuffer((unsigned char*)decodedData.data(), decodedData.size());
            textureObject->m_isDDS = false;
        }
        else
        {
            std::string fullPath = directory + '/' + path;

            auto it = m_TextureCache.find(image->uri);

            if (it != m_TextureCache.end())
            {
                textureObject = it->second;
            }
            else
            {
                
                
                std::string fileExtension = fullPath.substr(fullPath.find_last_of(".") + 1);
   
                textureObject = std::make_shared<Texture>(fullPath, fileExtension == "dds");
                m_TextureCache[image->uri] = textureObject;
            }
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
            loadTexture(material->normal_texture.texture, newMaterial.normalTexture);
        }
        if (pbr)
        {
            color = glm::vec4(pbr->base_color_factor[0], pbr->base_color_factor[1],
                pbr->base_color_factor[2], pbr->base_color_factor[3]);
            newMaterial.baseColor = color;
        }
        if (pbr->base_color_texture.texture)
        {
            //std::cout << "Has color texture" << "\n";
            newMaterial.hasColorTexture = true;
            loadTexture(pbr->base_color_texture.texture, newMaterial.colorTexture);
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

    std::cout << "Number of nodes: " << data->nodes_count << "\n";
    // Process Nodes
    for (cgltf_size i = 0; i < data->nodes_count; ++i) 
    {
        if (!data->nodes[i].parent) 
        { // Process root nodes
            processNode(&data->nodes[i]);
        }
    }

    std::cout << "Total amount of primitives: " << totalPrimitives << "\n";

    // Free the loaded data
    cgltf_free(data);
}

void cModel::processNode(cgltf_node* node, const glm::mat4& parentTransform)
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
    }
    else {
        glm::vec3 translation = node->has_translation ? glm::make_vec3(node->translation) : glm::vec3(0.0f);
        glm::quat rotation = node->has_rotation ? glm::make_quat(node->rotation) : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale = node->has_scale ? glm::make_vec3(node->scale) : glm::vec3(1.0f);

        nodeTransform = parentTransform * glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
    }

    if (node->mesh) {
        meshes.push_back(processMesh(node->mesh, nodeTransform));
    }

    for (cgltf_size i = 0; i < node->children_count; ++i) {
        processNode(node->children[i], nodeTransform);
    }
}


void cModel::extractAttributes(cgltf_primitive* primitive, cgltf_accessor*& positions, cgltf_accessor*& normals, cgltf_accessor*& texCoords0, cgltf_accessor*& texCoords1, cgltf_accessor*& tangents, cgltf_accessor*& colors)
{
    std::cout << "Number of primitive attributes: " << primitive->attributes_count << "\n";
    for (int i = 0; i < primitive->attributes_count; i++) 
    {
        std::cout << "Attribute: " << primitive->attributes[i].name << "\n";
        switch (primitive->attributes[i].type) 
        {
        case cgltf_attribute_type_position:
            positions = primitive->attributes[i].data;
            break;
        case cgltf_attribute_type_normal:
            normals = primitive->attributes[i].data;
            break;
        case cgltf_attribute_type_texcoord:
            if (strcmp(primitive->attributes[i].name, "TEXCOORD_0") == 0)
            {
                std::cout << "TEXCOORD_0 added" << "\n";
                texCoords0 = primitive->attributes[i].data;
            }
            if (strcmp(primitive->attributes[i].name, "TEXCOORD_1") == 0)
            {
                texCoords1 = primitive->attributes[i].data;
            }
            break;
        case cgltf_attribute_type_tangent:
            tangents = primitive->attributes[i].data;
            break;
        case cgltf_attribute_type_color:
            colors = primitive->attributes[i].data;
        default:
            break;
        }
    }
}

float* cModel::getBufferData(cgltf_accessor* accessor)
{
    if (!accessor || !accessor->buffer_view || !accessor->buffer_view->buffer) 
    {
        //std::cerr << "Invalid accessor or buffer view" << "\n";
        return nullptr;
    }
    return reinterpret_cast<float*>(static_cast<char*>(accessor->buffer_view->buffer->data) + accessor->buffer_view->offset + accessor->offset);
}




cMesh cModel::processMesh(cgltf_mesh* mesh, glm::mat4 transform)
{
    std::vector<cPrimitive> primitives;
    GLenum index_type = GL_UNSIGNED_SHORT;
    
    totalPrimitives += mesh->primitives_count;
    for (cgltf_size p = 0; p < mesh->primitives_count; ++p) 
    {
        try
        {
            cgltf_primitive* primitive = &mesh->primitives[p];
            cPrimitive newPrimitive = processPrimitive(primitive, index_type);
            newPrimitive.m_PrimIndex = p;
            primitives.push_back(std::move(newPrimitive));
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error processing primitive " << p << ": " << e.what() << "\n";
            continue;
        } 
    }


 
    cMesh newMesh(std::move(primitives));
    newMesh.transform = transform;
    return newMesh;
}

cPrimitive cModel::processPrimitive(cgltf_primitive* primitive, GLenum& index_type)
{
    cgltf_accessor* positions = nullptr;
    cgltf_accessor* v_normals = nullptr;
    cgltf_accessor* tex_coords0 = nullptr;
    cgltf_accessor* tex_coords1 = nullptr;
    cgltf_accessor* tangents = nullptr;
    cgltf_accessor* v_colors = nullptr;

    extractAttributes(primitive, positions, v_normals, tex_coords0, tex_coords1, tangents, v_colors);

    bool hasIndices = false;
  
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
            std::cerr << "Unknown index component type" << "\n";
            throw std::runtime_error("Unknown index component type");
        }
    }


    std::vector<float> interleavedData;
    interleavedData.reserve(positions->count * 11);

    // TODO
    if (primitive->has_draco_mesh_compression)
    {
        cgltf_draco_mesh_compression* draco_compression = &primitive->draco_mesh_compression;
        draco::DecoderBuffer draco_buffer;
        draco_buffer.Init(
            static_cast<const char*>(draco_compression->buffer_view->buffer->data) + draco_compression->buffer_view->offset,
            draco_compression->buffer_view->size
        );
        draco::Decoder decoder;
 
        std::unique_ptr<draco::Mesh> mesh = decoder.DecodeMeshFromBuffer(&draco_buffer).value();

        const draco::PointAttribute* pos_attr = mesh->GetNamedAttribute(draco::GeometryAttribute::POSITION);
        const draco::PointAttribute* norm_attr = mesh->GetNamedAttribute(draco::GeometryAttribute::NORMAL);
        const draco::PointAttribute* tex_attr = mesh->GetNamedAttribute(draco::GeometryAttribute::TEX_COORD);
        const draco::PointAttribute* tang_attr = mesh->GetNamedAttribute(draco::GeometryAttribute::TANGENT);
        

        std::vector<float> vertices;
        std::vector<float> normals;
        std::vector<float> texCoords;
        std::vector<float> tangents;


        if (pos_attr)
        {
            for (draco::AttributeValueIndex i(0); i < pos_attr->size(); ++i)
            {
                draco::Vector3f pos;
                pos_attr->GetValue(i, &pos);
                vertices.push_back(pos[0]);
                vertices.push_back(pos[1]);
                vertices.push_back(pos[2]);
            }
        }
        if (norm_attr)
        {
            for (draco::AttributeValueIndex i(0); i < norm_attr->size(); ++i)
            {
                draco::Vector3f norm;
                norm_attr->GetValue(i, &norm);
                normals.push_back(norm[0]);
                normals.push_back(norm[1]);
                normals.push_back(norm[2]);
            }
        }
        if (tex_attr)
        {
            for (draco::AttributeValueIndex i(0); i < tex_attr->size(); ++i)
            {
                draco::Vector2f tex;
                tex_attr->GetValue(i, &tex);
                texCoords.push_back(tex[0]);
                texCoords.push_back(tex[1]);
            }
        }
        if (tang_attr)
        {
            for (draco::AttributeValueIndex i(0); i < tang_attr->size(); ++i)
            {
                draco::Vector3f tang;
                tang_attr->GetValue(i, &tang);
                tangents.push_back(tang[0]);
                tangents.push_back(tang[1]);
                tangents.push_back(tang[2]);
            }
        }



        for (size_t i = 0; i < vertices.size() - 2; ++i)
        {
            interleavedData.insert(interleavedData.end(), {
                vertices[i], vertices[i + 1], vertices[i + 2],
                normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2],
                tang_attr ? tangents[i * 3] : 0.0f,
                tang_attr ? tangents[i * 3 + 1] : 0.0f,
                tang_attr ? tangents[i * 3 + 2] : 0.0f,
                tex_attr ? texCoords[i * 2] : 0.0f,
                tex_attr ? texCoords[i * 2 + 1] : 0.0f
             });
        }
    }
    else
    {
        float* vertices = getBufferData(positions);
        float* normData = getBufferData(v_normals);
        float* texData = getBufferData(tex_coords0);
        float* tangData = getBufferData(tangents);



        if (!vertices || !normData)
        {
            std::cerr << "Error accessing buffer data for positions or normals" << "\n";
            throw std::runtime_error("Error accessing buffer data for positions or normals");
        }

        if (positions->buffer_view->stride == 24)
        {
            for (size_t i = 0; i < positions->count; ++i)
            {
                interleavedData.insert(interleavedData.end(), {
                    vertices[i * 6], vertices[i * 6 + 1], vertices[i * 6 + 2],
                    vertices[(i-1) * 6 + 3], vertices[(i-1) * 6 + 4], vertices[(i-1) * 6 + 5],
                    tangData ? tangData[i * 3] : 0.0f,
                    tangData ? tangData[i * 3 + 1] : 0.0f,
                    tangData ? tangData[i * 3 + 2] : 0.0f,
                    texData ? texData[i * 2] : 0.0f,
                    texData ? texData[i * 2 + 1] : 0.0f
                    });
            }
        }
        else
        {
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
        }


    }
    

    void* bufferData = getBufferData(primitive->indices);

 

    std::vector<unsigned int> indices;
    if (primitive->indices)
    {
        hasIndices = true;
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
    else
    {
        std::cout << "No indices" << "\n";
    }
    

    Material newMaterial;
    try
    {
        newMaterial = createMaterial(primitive);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        throw;
    }
    
    return cPrimitive(std::move(interleavedData), std::move(indices), index_type, newMaterial, hasIndices);
}



void cModel::uploadToGpu(Shader& shader)
{
 
    for (auto& mesh : meshes)
    {
        //shader.setMat4("model", mesh.transform);
        mesh.transformChanged = true;
        mesh.uploadToGpu();
    }
    std::cout << "Number of texture" << m_TextureCache.size() << "\n";
    for (const auto& pair : m_TextureCache) {
        // pair is of type std::pair<const std::string, std::shared_ptr<Texture>>
        pair.second->clearData();
    }
    
}

glm::vec3 transformVertex(const glm::vec3& vertex, const glm::mat4& transform)
{
    return glm::vec3(transform * glm::vec4(vertex, 1.0f));
}

void cModel::batchTest()
{
    unsigned int vertexOffset = 0;

    for (auto& mesh : meshes)
    {
        mesh.combinePrimitiveData();

        // Apply transformation to each vertex in the mesh
        for (size_t i = 0; i < mesh.m_CombinedInterleavedData.size(); i += 11) {
            glm::vec3 vertex(mesh.m_CombinedInterleavedData[i], mesh.m_CombinedInterleavedData[i + 1], mesh.m_CombinedInterleavedData[i + 2]);
            vertex = transformVertex(vertex, mesh.transform);

            // Update the vertex position in the interleaved data
            mesh.m_CombinedInterleavedData[i] = vertex.x;
            mesh.m_CombinedInterleavedData[i + 1] = vertex.y;
            mesh.m_CombinedInterleavedData[i + 2] = vertex.z;
        }

        m_CombinedInterleavedData.insert(m_CombinedInterleavedData.end(), mesh.m_CombinedInterleavedData.begin(), mesh.m_CombinedInterleavedData.end());

        for (auto index : mesh.m_CombinedIndices)
        {
            m_CombinedIndices.push_back(static_cast<unsigned int>(index) + vertexOffset);
        }


        vertexOffset += mesh.m_CombinedInterleavedData.size() / 11;
    }

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error during " << "message" << ": " << err << std::endl;
    }
    GLuint EBO, VBO;
    glGenVertexArrays(1, &this->m_VAO);
    glBindVertexArray(this->m_VAO);

    // Interleaved VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, m_CombinedInterleavedData.size() * sizeof(float), m_CombinedInterleavedData.data(), GL_STATIC_DRAW);

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

    std::vector<unsigned short> temp_indices(m_CombinedIndices.begin(), m_CombinedIndices.end());
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_CombinedIndices.size() * sizeof(unsigned int), m_CombinedIndices.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void cModel::renderModelBatch(Shader& shader)
{
    glBindVertexArray(m_VAO);

    glDrawElements(GL_TRIANGLES, // mode: specifies the kind of primitives to render
        m_CombinedIndices.size(), // count: specifies the number of elements to be rendered
        GL_UNSIGNED_INT, // type: specifies the type of the values in indices
        0); // indices: specifies a pointer to the location where the indices are stored (NULL if EBO is bound)


    // Unbind VAO to avoid accidentally modifying it
    glBindVertexArray(0);
}