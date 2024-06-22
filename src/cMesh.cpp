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
