

#version 330 core
out vec4 FragColor;

struct Material {
    vec3 baseColor;
    bool hasTexture;
    sampler2D diffuse;
    sampler2D normal;
    sampler2D specular;    
    float shininess;
    sampler2D emission;
}; 

struct Light {
    vec3  position;
    vec3  direction;
    float cutOff;
    float outerCutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};


struct DirLight {
    vec3 direction;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool enabled;
};  
uniform DirLight dirLight;

struct PointLight {    
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;  

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool enabled;
};  
#define NR_POINT_LIGHTS 4  
uniform PointLight pointLights[NR_POINT_LIGHTS];

struct SpotLight {
    vec3  position;
    vec3  direction;
    float cutOff;
    float outerCutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
    bool enabled;
};
uniform SpotLight spotLight;


in vec3 FragPos;  
in vec3 Normal;  
in vec3 Tangent;
in vec3 Bitangent;
in vec2 TexCoords;
in vec3 TangentFragPos;
in vec3 TangentViewPos;
  
uniform vec3 viewPos;
uniform Material material;
uniform Light light;
uniform bool useNormalTexture;


float near = 0.5;
float far = 100.0;



vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);  
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);  
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float LinearizeDepth(float depth);


void main()
{
    //FragColor = texture(material.specular, TexCoords);
    FragColor = vec4(Normal, 1.0);
    return;
    vec3 viewDir = normalize(TangentViewPos - TangentFragPos);

    vec4 textureColour;
    if(material.hasTexture)
    {
        textureColour = texture(material.specular, TexCoords);
    }
    else 
    {
        textureColour = vec4(material.baseColor, 1.0);
    }
    if (textureColour.a < 0.1)
    {
        discard;
    }

    vec3 normalColor = texture(material.normal, TexCoords).rgb;

    vec3 N = normalize((normalColor * 2.0 - 1.0) * 10.0);

    // Calculate TBN matrix
    vec3 T = normalize(Tangent);
    vec3 B = normalize(Bitangent);
    vec3 N_world = normalize(Normal);
    mat3 TBN = mat3(T, B, N_world);
  
    vec3 normal;
    if(useNormalTexture)
    {
        normal = normalize(TBN * N);
    }
    else
    {
        normal = normalize(N_world);
    }

    vec3 result = vec3(0.0f);
    // phase 1: Directional lighting
    if (dirLight.enabled)
        result = CalcDirLight(dirLight, normal, viewDir);


    FragColor = vec4(textureColour.rgb, 1.0);
}



vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // combine results
    vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    return (specular);
}  

