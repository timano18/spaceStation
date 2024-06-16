#pragma once

class Material {
public: 
	glm::vec4  baseColor = glm::vec4(1.0);
	GLuint colorTextureID;
	GLuint normalTextureID;
	GLuint aoTextureID;

	Material() : baseColor(1.0f, 1.0f, 1.0f, 1.0f)
	{
	}
	Material(glm::vec4 nBaseColor)
	{
		baseColor = nBaseColor;
	}
};