#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>

class Material
{
public:
    glm::vec4 baseColor;
    GLuint colorTextureID;
    GLuint normalTextureID;
    GLuint aoTextureID;
    bool hasTexture;

    Material();
    Material(glm::vec4 nBaseColor);
};
