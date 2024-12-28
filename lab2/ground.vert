// ground.vert
#version 330 core

// Input
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexUV;

// Output data, to be interpolated for each fragment
out vec3 color;
out vec2 uv;

out vec3 worldNormal;
out vec3 worldPosition;

// TODO: To add UV to this vertex shader 

// Matrix for vertex transformation
uniform mat4 MVP;
uniform mat4 model;        // Model matrix to transform positions
uniform mat3 normalMatrix; // Normal matrix to transform normals

void main() {
    vec3 transformNormal;
    vec4 transformPosition;

    worldPosition = (model * vec4(vertexPosition, 1.0)).xyz;
    
    //worldNormal = normalMatrix * vertexNormal;
    worldNormal = vertexNormal;

    // Pass vertex color to the fragment shader
    //color = vertexColor;

    // TODO: Pass UV to the fragment shader
    uv = vertexUV;    

    // Transform vertex
    gl_Position =  MVP * vec4(vertexPosition, 1.0);

}
