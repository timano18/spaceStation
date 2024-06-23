#pragma once
#include <glad/glad.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdint>
#include "MipmapData.h"
#include <fstream>
#include <thread>

struct DDSHeader
{
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitchOrLinearSize;
    uint32_t depth;
    uint32_t mipMapCount;
    uint32_t reserved1[11];

    struct
    {
        uint32_t size;
        uint32_t flags;
        uint32_t fourCC;
        uint32_t RGBBitCount;
        uint32_t RBitMask;
        uint32_t GBitMask;
        uint32_t BBitMask;
        uint32_t ABitMask;
    } pixelFormat;

    struct
    {
        uint32_t caps1;
        uint32_t caps2;
        uint32_t caps3;
        uint32_t caps4;
    } caps;

    uint32_t reserved2;
};

struct DDSHeaderDX10
{
    uint32_t dxgiFormat;
    uint32_t resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t miscFlags2;
};

class Texture {
public:
    GLenum m_format;
    int m_width;
    int m_height;
    std::vector<unsigned char> m_data;
    std::vector<MipmapData> m_ddsData;
    bool m_isDDS;

    Texture(const std::string& path, bool isDDS)
    {

        m_isDDS = isDDS;
        if (!m_isDDS)
        {
            loadStandardTexture(path);
        }
        else
        {
            loadDDSTexture(path);
        }
 
    }
    Texture()
    {
    }

    std::vector<MipmapData> readDDS(std::string filePath, DDSHeader& header, DDSHeaderDX10& headerDX10)
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open DDS file");
        }

        // Read magic number
        char magic[4];
        file.read(magic, sizeof(magic));
        if (file.gcount() != sizeof(magic) || std::strncmp(magic, "DDS ", sizeof(magic)) != 0)
        {
            throw std::runtime_error("Not a valid DDS file: " + filePath);
        }

        // Read header
        file.read(reinterpret_cast<char*>(&header), sizeof(DDSHeader));
        if (file.gcount() != sizeof(DDSHeader))
        {
            throw std::runtime_error("Failed to read DDS header: " + filePath);
        }

        // Check for DX10 header and read it if present
        if (header.pixelFormat.fourCC == 0x30315844)
        {
            file.read(reinterpret_cast<char*>(&headerDX10), sizeof(DDSHeaderDX10));
            if (file.gcount() != sizeof(DDSHeaderDX10))
            {
                throw std::runtime_error("Failed to read DX10 header: " + filePath);
            }
        }

        std::vector<MipmapData> mipmaps;
        uint32_t width = header.width;
        uint32_t height = header.height;

        for (uint32_t level = 0; level < header.mipMapCount; ++level)
        {
            size_t dataSize = ((width + 3) / 4) * ((height + 3) / 4) * 16; // BC7 block size is 16 bytes
            mipmaps.push_back({ width, height, std::vector<uint8_t>(dataSize) });

            file.read(reinterpret_cast<char*>(mipmaps[level].data.data()), dataSize);
            if (file.gcount() != dataSize)
            {
                throw std::runtime_error("Failed to read mipmap level data: " + filePath);
            }

            width = std::max(1U, width / 2);
            height = std::max(1U, height / 2);
        }

        return mipmaps;
    }

    void loadDDSTexture(std::string path)
    {
        try
        {
            DDSHeader header;
            DDSHeaderDX10 headerDX10 = {};
            m_ddsData = readDDS(path, header, headerDX10);

            GLenum format = 0;

            // Constants
            const uint32_t FOURCC_DXT1 = 0x31545844; // 'DXT1'
            const uint32_t FOURCC_DXT3 = 0x33545844; // 'DXT3'
            const uint32_t FOURCC_DXT5 = 0x35545844; // 'DXT5'
            const uint32_t FOURCC_DX10 = 0x30315844; // 'DX10'
            const uint32_t DXGI_FORMAT_BC7_UNORM = 98;
            const uint32_t DXGI_FORMAT_BC7_UNORM_SRGB = 99;

            switch (header.pixelFormat.fourCC) {
            case FOURCC_DXT1:
                format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                break;
            case FOURCC_DXT3:
                format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                break;
            case FOURCC_DXT5:
                format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                break;
            case FOURCC_DX10:
                if (headerDX10.dxgiFormat == DXGI_FORMAT_BC7_UNORM)
                {
                    format = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
                }
                else if (headerDX10.dxgiFormat == DXGI_FORMAT_BC7_UNORM_SRGB)
                {
                    format = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB;
                }
                else
                {
                    throw std::runtime_error("Unsupported DXGI format in DX10 header");
                }
                break;
            default:
                throw std::runtime_error("Unsupported DDS format");
            }
            m_format = format;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error loading DDS texture: " << e.what() << std::endl;
        }
    }

    void loadStandardTexture(const std::string& path)
    {
        int nrChannels;
        unsigned char* rawData = stbi_load(path.c_str(), &m_width, &m_height, &nrChannels, 0);
        if (rawData == nullptr) {
            throw std::runtime_error("Failed to load image: " + path);
        }
        // Copy data to vector and free raw data
        m_data.assign(rawData, rawData + (m_width * m_height * nrChannels));
        //stbi_image_free(rawData);
        switch (nrChannels)
        {
        case 1: m_format = GL_RED; break;
        case 3: m_format = GL_RGB; break;
        case 4: m_format = GL_RGBA; break;
        default:
            std::cerr << "Unsupported number of channels: " << nrChannels << std::endl;
        }
    }
};