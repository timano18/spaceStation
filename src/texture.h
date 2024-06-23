#pragma once
#include <glad/glad.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdint>



class Texture {
public:
    GLenum format;
    int width;
    int height;
   // std::vector<MipmapData> mipmaps;
    bool isDDS;

    Texture(GLenum format, int width, int height, bool isDDS)
        : format(format), width(width), height(height), isDDS(isDDS)
    {
    }
    Texture()
    {
    }
};