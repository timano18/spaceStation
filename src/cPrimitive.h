#pragma once
#include "pch.h"
#include "cMaterial.h"
#include <glad/glad.h>
#include <cgltf/cgltf.h>
#include "shader.h"

class cPrimitive 
{
public:
    //GLuint VAO, VBO, EBO;
    std::vector<float> m_interleavedData;
    std::vector<unsigned int> m_indices;
  
    GLenum m_indexType;
    Material m_material;
    GLuint m_VAO;


    cPrimitive(std::vector<float> interleavedData, std::vector<unsigned int> indices, GLenum nIndex_type, Material nMaterial);

    void draw(Shader& shader);

    void uploadToGPU();
};
