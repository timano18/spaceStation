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
    
    std::shared_ptr<Texture> colorTexture;
    std::shared_ptr<Texture> normalTexture;

    bool hasColorTexture;

    Material();
    Material(glm::vec4 nBaseColor);
    GLuint createOpenGLTexture(const std::shared_ptr<Texture>& texture);
    void createAllTextures();
};
