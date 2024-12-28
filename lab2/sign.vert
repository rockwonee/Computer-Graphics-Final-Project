#version 330 core
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexUV;

out vec2 uv;
out vec3 worldNormal;
out vec3 worldPosition;

uniform mat4 MVP;
uniform mat4 model;
uniform mat3 normalMatrix;

void main() {
    worldPosition = (model * vec4(vertexPosition, 1.0)).xyz;

    worldNormal = normalMatrix * vec3(0.0, 0.0, 1.0); // for a flat surface

    uv = vertexUV; 

    gl_Position = MVP * vec4(vertexPosition, 1.0);
}
