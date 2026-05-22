
#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor; 
uniform vec3 emission;
uniform sampler2D planetTexture;     
uniform sampler2D texture_specular1; 
uniform bool useTexture; 
uniform bool useSpecularMap;         

void main() {

    vec3 baseColor = objectColor;

    if(useTexture) {
        baseColor = texture(planetTexture, TexCoords).rgb;
    }

    float ambientStrength = 0.1;

    vec3 ambient = ambientStrength * baseColor;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);

    vec3 diffuse = diff * baseColor;

    vec3 specular = vec3(0.0);

    if(useSpecularMap && diff > 0.0) { 
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);          
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

        vec3 specularMask = texture(texture_specular1, TexCoords).rgb;

        specular = spec * specularMask; 
    }
    
    vec3 selfLight = emission * baseColor;

    vec3 result = ambient + diffuse + specular + selfLight;

    FragColor = vec4(result, 1.0);
}