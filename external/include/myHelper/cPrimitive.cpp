#include "cPrimitive.h"

cPrimitive::cPrimitive(GLuint nVAO, cgltf_size nGlobalIndexCount, GLenum nIndex_type, Material nMaterial)
    : VAO(nVAO), globalIndexCount(nGlobalIndexCount), index_type(nIndex_type), material(nMaterial)
{
}

void cPrimitive::draw(Shader& shader)
{
    shader.use();
    // shader.setVec3("objectColor", material.baseColor.x, material.baseColor.y, material.baseColor.z);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, material.colorTextureID);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, material.normalTextureID);
    shader.setInt("material.diffuse", 0);
    shader.setInt("material.normal", 1);
    shader.setVec3("material.baseColor", material.baseColor);
    shader.setBool("material.hasTexture", material.hasTexture);
    glBindVertexArray(VAO);

    glDrawElements(GL_TRIANGLES, // mode: specifies the kind of primitives to render
        globalIndexCount, // count: specifies the number of elements to be rendered
        index_type, // type: specifies the type of the values in indices
        0); // indices: specifies a pointer to the location where the indices are stored (NULL if EBO is bound)

    // Unbind the VAO to prevent accidental modifications
    glBindVertexArray(0);
}
