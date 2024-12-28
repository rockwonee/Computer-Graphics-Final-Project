#version 330 core

// Input
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexUV;
layout(location = 3) in vec4 vertexJointIndices;
layout(location = 4) in vec4 vertexJointWeights;

// Output data, to be interpolated for each fragment
out vec3 worldNormal;
out vec3 worldPosition;


uniform mat4 MVP;
uniform mat4 lightSpaceMatrix; // Light-space transformation matrix

uniform mat4 jointMatrices[70];
uniform bool useSkinning;  

void main() {
    vec3 transformNormal;
    vec4 transformPosition;

    if (useSkinning) {
        // Skinning logic
        transformNormal = vec3(0.0);
        transformPosition = vec4(0.0);
        
        for (int j = 0; j < 6; j++) { // GLTF typically supports up to 4 weights per vertex
            int jointIndex = int(vertexJointIndices[j]);
            float weight = vertexJointWeights[j];

            transformPosition += weight * (jointMatrices[jointIndex] * vec4(vertexPosition, 1.0));
            transformNormal += weight * mat3(jointMatrices[jointIndex]) * vertexNormal;
        }

        // Normalize the normal vector
        transformNormal = normalize(transformNormal);

    } else {
        // No skinning: Direct transformation
        transformPosition = vec4(vertexPosition, 1.0);
        transformNormal = vertexNormal;
    }

    // Transform vertex
    gl_Position =  MVP * transformPosition;

    // World-space geometry 
    worldPosition = transformPosition.xyz;
    worldNormal = normalize(transformNormal);

}
