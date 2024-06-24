#pragma once

#include "cPrimitive.h"
#include <vector>
#include "shader.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class cMesh 
{
public:
    std::vector<cPrimitive> primitives;
    glm::mat4 transform;
    bool transformChanged;
    cMesh(const std::vector<cPrimitive>& primitives);
    std::vector<float> m_CombinedInterleavedData;
    std::vector<unsigned int> m_CombinedIndices;
    std::unordered_map<GLuint, Material> m_PrimMateriallsMap;
    std::vector<Texture> m_ColorTextures;
    GLuint m_TextureID;
    GLuint m_VAO;
    

    void draw(Shader& shader);
    void uploadToGpu();
    void combinePrimitiveData();
    void renderBatch(Shader& shader);
};




