

#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 Bitangent;
out vec2 TexCoords;
out vec3 TangentFragPos;
out vec3 TangentViewPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;



void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;  
    Tangent = mat3(transpose(inverse(model))) * aTangent;
    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    Bitangent = cross(Normal, Tangent);
    TexCoords = aTexCoords;

    mat3 TBN = transpose(mat3(Tangent, Bitangent, Normal));
    TangentFragPos = TBN * FragPos;
    TangentViewPos = TBN * viewPos;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}

