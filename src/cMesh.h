#pragma once

#include "cPrimitive.h"
#include <vector>
#include "shader_s.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class cMesh {
public:
    std::vector<cPrimitive> primitives;
    glm::mat4 transform;

    cMesh(const std::vector<cPrimitive>& primitives);

    void draw(Shader& shader);
};




