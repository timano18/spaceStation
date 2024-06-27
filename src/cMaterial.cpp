#include "pch.h"
#include "cMaterial.h"
#include "texture.h"

Material::Material() : baseColor(1.0f, 1.0f, 1.0f, 1.0f) {}

Material::Material(glm::vec4 nBaseColor) : baseColor(nBaseColor){}

GLuint Material::createOpenGLTexture(const std::shared_ptr<Texture>& texture)
{

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    if (texture->m_isDDS)
    {
        for (size_t level = 0; level < texture->m_ddsData.size(); ++level)
        {
            const auto& mipmap = texture->m_ddsData[level];
            glCompressedTexImage2D(GL_TEXTURE_2D, level, texture->m_format, mipmap.width, mipmap.height, 0, mipmap.data.size(), mipmap.data.data());
            //checkGLError("texture upload at mipmap level " + std::to_string(level));
        }
    }
    else
    {


        glTexImage2D(GL_TEXTURE_2D, 0, texture->m_format, texture->m_width, texture->m_height, 0, texture->m_format, GL_UNSIGNED_BYTE, texture->m_data.data());
         glGenerateMipmap(GL_TEXTURE_2D);
    }


    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);



    return textureID;
}

void Material::createAllTextures()
{
    colorTextureID = createOpenGLTexture(this->colorTexture);
}