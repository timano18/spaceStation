#include "pch.h"
#include "cMesh.h"

cMesh::cMesh(const std::vector<cPrimitive>& primitives)
    : primitives(primitives)
{

}

void cMesh::draw(Shader& shader)
{
    for (auto& primitive : primitives) 
    {
        primitive.draw(shader);
    }
}

void cMesh::uploadToGpu()
{
    for (auto& primitive : primitives)
    {
        primitive.uploadToGPU();

    }
}

void cMesh::combinePrimitiveData()
{
    unsigned int vertexOffset = 0;

    for (auto& primitive : primitives)
    {
        m_CombinedInterleavedData.insert(m_CombinedInterleavedData.end(), primitive.m_interleavedData.begin(), primitive.m_interleavedData.end());
     
        for (auto index : primitive.m_indices)
        {
            m_CombinedIndices.push_back(static_cast<unsigned int>(index) + vertexOffset);
        }


        vertexOffset += primitive.m_interleavedData.size() / 11;
    }

    /*

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error during " << "message" << ": " << err << std::endl;
    }
    GLuint EBO, VBO;
    glGenVertexArrays(1, &this->m_VAO);
    glBindVertexArray(this->m_VAO);

    // Interleaved VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, m_CombinedInterleavedData.size() * sizeof(float), m_CombinedInterleavedData.data(), GL_STATIC_DRAW);

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

    std::vector<unsigned short> temp_indices(m_CombinedIndices.begin(), m_CombinedIndices.end());
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_CombinedIndices.size() * sizeof(unsigned int), m_CombinedIndices.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
    */
}



void cMesh::renderBatch(Shader& shader)
{


    

    glBindVertexArray(m_VAO);

    glDrawElements(GL_TRIANGLES, // mode: specifies the kind of primitives to render
        m_CombinedIndices.size(), // count: specifies the number of elements to be rendered
        GL_UNSIGNED_INT, // type: specifies the type of the values in indices
        0); // indices: specifies a pointer to the location where the indices are stored (NULL if EBO is bound)


    // Unbind VAO to avoid accidentally modifying it
    glBindVertexArray(0);
}


