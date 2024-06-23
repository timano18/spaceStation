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
    


    bool hasColorTexture;

    Material();
    Material(glm::vec4 nBaseColor);
    void uploadTexture();
};
