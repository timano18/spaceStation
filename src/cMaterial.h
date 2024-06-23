#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "texture.h"


class Material
{
public:
    glm::vec4 baseColor;
    GLuint colorTextureID;
    GLuint normalTextureID;
    GLuint aoTextureID;
    
    Texture colorTexture;
    Texture normalTexture;

    bool hasColorTexture;

    Material();
    Material(glm::vec4 nBaseColor);
    GLuint createOpenGLTexture(const Texture& texture);
    void createAllTextures();
};
