#version 330 core
out vec4 FragColor;

uniform vec3 lightColor;
uniform float intensity;

in vec2 TexCoord;

void main() {
    float glow = 1.0 - length(TexCoord - vec2(0.5)); // Radial fade
    FragColor = vec4(lightColor * glow * intensity, glow);
}
