#include "pch.h"
#include "cPrimitive.h"

void checkGLError(const std::string& message)
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error during " << message << ": " << err << "\n";
    }
}
cPrimitive::cPrimitive(std::vector<float> interleavedData, std::vector<unsigned int> indices, GLenum nIndex_type, Material nMaterial, bool hasIndices)
    : m_interleavedData(interleavedData), m_indices(indices), m_indexType(nIndex_type), m_material(nMaterial), m_HasIndices(hasIndices)
{
}

void cPrimitive::draw(Shader& shader)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_material.colorTextureID);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_material.normalTextureID);


    shader.setInt("material.diffuse", 0);
    shader.setInt("material.normal", 1);
    shader.setVec3("material.baseColor", m_material.baseColor);
    shader.setBool("material.hasTexture", m_material.hasColorTexture);
    glBindVertexArray(this->m_VAO);

    if (m_HasIndices) 
    {
        glDrawElements(GL_TRIANGLES, // mode: specifies the kind of primitives to render
            m_indices.size(), // count: specifies the number of elements to be rendered
            m_indexType, // type: specifies the type of the values in indices
            0); // indices: specifies a pointer to the location where the indices are stored (NULL if EBO is bound)
    }
    else
    {
        std::cout << "Implement glDrawArrays" << "\n";
    }

    // Unbind the VAO to prevent accidental modifications
    glBindVertexArray(0);
}

void cPrimitive::uploadToGPU()
{
    if (m_material.hasColorTexture)
    {
        m_material.createAllTextures();
    }

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error during " << "message" << ": " << err << "\n";
    }
    GLuint EBO, VBO;
    glGenVertexArrays(1, &this->m_VAO);
    glBindVertexArray(this->m_VAO);

    // Interleaved VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, m_interleavedData.size() * sizeof(float), m_interleavedData.data(), GL_STATIC_DRAW);
    
    const size_t stride = 11 * sizeof(float); // 3 (position) + 3 (normal) + 3 (tangent) + 2 (texcoord) 
    // Positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    // Normals
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Tangents
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    // Texture Coordinates
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)(9 * sizeof(float)));
    glEnableVertexAttribArray(3);
    // Indices
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    switch (m_indexType)
    {
        case GL_UNSIGNED_INT:
        {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);
            break;
        }
        case GL_UNSIGNED_SHORT:
        {
            std::vector<unsigned short> temp_indices(m_indices.begin(), m_indices.end());
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, temp_indices.size() * sizeof(unsigned short), temp_indices.data(), GL_STATIC_DRAW);
            break;
        }  
        case GL_UNSIGNED_BYTE: 
        {
            std::vector<unsigned char> temp_indices(m_indices.begin(), m_indices.end());
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, temp_indices.size() * sizeof(unsigned char), temp_indices.data(), GL_STATIC_DRAW);
            break;
        }

        default:
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);

    }


    // Unbind VAO to avoid accidentally modifying it
    glBindVertexArray(0);
}
