#pragma once
#include "cMaterial.h"
#include <glad/glad.h>
#include <cgltf/cgltf.h>
#include "shader_s.h"

class cPrimitive {
public:
    GLuint VAO, VBO, EBO;
    cgltf_size globalIndexCount;
    GLenum index_type;
    Material material;

    cPrimitive(GLuint nVAO, cgltf_size nGlobalIndexCount, GLenum nIndex_type, Material nMaterial);

    void draw(Shader& shader);
};
