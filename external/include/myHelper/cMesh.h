#pragma once

#include <myHelper/cPrimitive.h>


class cMesh {
public:
    std::vector<cPrimitive> primitives;
    glm::mat4 transform;

    cMesh(const std::vector<cPrimitive>& primitives)
        : primitives(primitives) { }

    void draw(Shader& shader)
    {
        for (auto& primitive : primitives) {
            primitive.draw(shader);
        }
       
    }
};




