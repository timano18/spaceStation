#pragma once

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
#include "shader.h"
#include "cMesh.h"
#include <future>

struct DDSHeader;
struct DDSHeaderDX10;
struct MipmapData;


class cModel 
{
public:
    std::vector<cMesh> meshes;
    std::string directory;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_TextureCache;

    std::vector<float> m_CombinedInterleavedData;
    std::vector<unsigned int> m_CombinedIndices;

    GLuint m_VAO;


    cModel(const char* path);
    void Draw(Shader& shader, bool useBatchRendering);
    std::vector<MipmapData> readDDS(const std::string& filePath, DDSHeader& header, DDSHeaderDX10& headerDX10);
    void checkGLError(const std::string& message);
    GLuint loadDDSTexture(const std::string& path);
    Texture loadStandardTexture(const std::string& path);
    void loadTexture(cgltf_texture* texture, std::shared_ptr<Texture>& textureObject);
    Material createMaterial(cgltf_primitive* primitive);
    void loadModel(const char* path);
    void processNode(cgltf_node* node, const glm::mat4& parentTransform = glm::mat4(1.0f));
    void extractAttributes(cgltf_primitive* primitive, cgltf_accessor*& positions, cgltf_accessor*& normals, cgltf_accessor*& texCoords, cgltf_accessor*& tangents);
    float* getBufferData(cgltf_accessor* accessor);
    cMesh processMesh(cgltf_mesh* mesh, glm::mat4 transform);
    cPrimitive processPrimitive(cgltf_primitive* primitive, GLenum& index_type);
    void uploadToGpu(Shader& shader);
    void batchTest();
    void renderModelBatch(Shader& shader);

};


