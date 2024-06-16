#pragma once
#include <myHelper/cMaterial.h>

class cPrimitive {
public:
    GLuint VAO, VBO, EBO;
    cgltf_size globalIndexCount;
    GLenum index_type;
    Material material;

    cPrimitive(GLuint nVAO, cgltf_size nGlobalIndexCount, GLenum nIndex_type, Material nMaterial)
    {
        VAO = nVAO;
        globalIndexCount = nGlobalIndexCount;
        index_type = nIndex_type;
        material = nMaterial;
    }

    void draw(Shader& shader)
    {
        shader.use();
       // shader.setVec3("objectColor", material.baseColor.x, material.baseColor.y, material.baseColor.z);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material.colorTextureID);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, material.normalTextureID);
        GLint textureUniformLocation = glGetUniformLocation(shader.ID, "material.diffuse");
        glUniform1i(textureUniformLocation, 0);
        GLint textureUniformLocation2 = glGetUniformLocation(shader.ID, "material.normal");
        glUniform1i(textureUniformLocation2, 1);
        glBindVertexArray(VAO);
        
       
        glDrawElements(GL_TRIANGLES, // mode: specifies the kind of primitives to render
            globalIndexCount, // count: specifies the number of elements to be rendered
            index_type, // type: specifies the type of the values in indices
            0); // indices: specifies a pointer to the location where the indices are stored (NULL if EBO is bound)

        // Unbind the VAO to prevent accidental modifications
        glBindVertexArray(0);
       
    }
};




