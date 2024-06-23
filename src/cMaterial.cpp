#include "pch.h"
#include "cMaterial.h"
#include "texture.h"

Material::Material() : baseColor(1.0f, 1.0f, 1.0f, 1.0f) {}

Material::Material(glm::vec4 nBaseColor) : baseColor(nBaseColor){}

void Material::uploadTexture()
{

}
