#version 330 core

in vec3 worldPosition;
in vec3 worldNormal; 

out vec3 finalColor;

uniform vec3 lightPosition;
uniform vec3 lightIntensity;

// Fog 
const uniform vec3 fogColor = vec3(0.3, 0.3, 0.4);        
uniform float fogStart = 100.0;       // Start distance for fog
uniform float fogEnd = 500.0;         // End distance for fog
uniform float fogDensity = 0.02f;
uniform vec3 cameraPosition;  // Camera position

void main()
{
	// Lighting
	vec3 lightDir = lightPosition - worldPosition;
	float lightDist = dot(lightDir, lightDir);
	lightDir = normalize(lightDir);
	vec3 v = lightIntensity * clamp(dot(lightDir, worldNormal), 0.0, 1.0) / lightDist;

	// Tone mapping
	v = v / (1.0 + v);

	// Gamma correction
    vec3 litColor = pow(v, vec3(1.0 / 2.2)); 

    // Fog calculation
    float distance = length(worldPosition - cameraPosition); // Distance to the camera
    //float fogFactor = clamp((distance - fogStart) / (fogEnd - fogStart), 0.0, 1.0);

    float fogFactor = exp(-distance * fogDensity);
    fogFactor = clamp(fogFactor, 0.0, 1.0);


    // Blend the lit color with the fog color
    //finalColor = mix(fogColor, litColor, fogFactor);

    //finalColor = vec3(distance / 500.0); // Normalize distance for visualization

    //finalColor = vec3(fogFactor); // Visualize fog factor as grayscale

    
    //finalColor = vec3(1.0, 0.0, 0.0);

    finalColor = litColor; // WORKING 

    //finalColor = worldPosition;
}