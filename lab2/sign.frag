#version 330 core


in vec2 uv;
in vec3 worldNormal;
in vec3 worldPosition;

uniform sampler2D textureSampler;

// Sphere light uniforms
uniform vec3 sphereLightPos;
uniform vec3 sphereLightColor;
uniform float sphereLightIntensity;

uniform vec3 viewPos;

out vec3 finalColor;

void main() {

    vec3 textureColor = texture(textureSampler, uv).rgb;
    vec3 norm = normalize(worldNormal);

    vec3 lightDir = normalize(sphereLightPos - worldPosition);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = textureColor * sphereLightColor * diff * sphereLightIntensity;

    vec3 viewDir = normalize(viewPos - worldPosition);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); 
    vec3 specular = vec3(1.0) * spec * sphereLightColor * sphereLightIntensity;

    float distance = length(sphereLightPos - worldPosition);
    float attenuation = 1.0 / (1.0 + 0.01 * distance + 0.001 * (distance * distance));
    diffuse *= attenuation;
    specular *= attenuation;

    finalColor = diffuse + specular;
}
