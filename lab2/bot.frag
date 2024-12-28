#version 330 core

in vec3 worldPosition;
in vec3 worldNormal; 



uniform vec3 lightPosition;
uniform vec3 lightIntensity;


// Fog 
const uniform vec3 fogColor = vec3(0.3, 0.3, 0.4);        
uniform float fogStart = 100.0;       // Start distance for fog
uniform float fogEnd = 500.0;         // End distance for fog
uniform float fogDensity = 0.02f;
uniform vec3 cameraPosition;  // Camera position

// Sphere light uniforms
uniform vec3 sphereLightPos;      
uniform vec3 sphereLightColor;    
uniform float sphereLightIntensity;

uniform vec3 viewPos;

out vec3 finalColor;


void main()
{
	// Lighting
    /*
	vec3 lightDir = lightPosition - worldPosition;
	float lightDist = dot(lightDir, lightDir);
	lightDir = normalize(lightDir);
	vec3 v = lightIntensity * clamp(dot(lightDir, worldNormal), 0.0, 1.0) / lightDist;

	// Tone mapping
	v = v / (1.0 + v);
    

	// Gamma correction
    vec3 litColor = pow(v, vec3(1.0 / 2.2)); 
    */
    // GLOW FROM SPHERE
    vec3 norm = normalize(worldNormal);
    vec3 lightDir = normalize(sphereLightPos - worldPosition);

    // Diffuse lighting
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = sphereLightColor * diff * sphereLightIntensity;

    vec3 viewDir = normalize(viewPos - worldPosition);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); // Shininess factor is 32.0
    vec3 specular = vec3(1.0) * spec * sphereLightColor * sphereLightIntensity;

    float distance = length(sphereLightPos - worldPosition);
    float attenuation = 1.0 / (1.0 + 0.01 * distance + 0.001 * (distance * distance));

    diffuse *= attenuation;
    specular *= attenuation;

    //vec3 ambientLightColor = vec3(0.2, 0.2, 0.2); 
    //vec3 ambient = ambientLightColor; 

    // Combine texture color with lighting
    finalColor = diffuse + specular; // WORKING ALSO

    //finalColor = ambient + diffuse + specular;
    finalColor = finalColor / (1.0 + finalColor); // Tone mapping
    finalColor = pow(finalColor, vec3(1.0 / 1.2)); // Gamma correction

    vec3 glowCol = diffuse + specular;

    //finalColor = litColor; // WORKING 

    // Fog calculation
    //float distance = length(worldPosition - cameraPosition); // Distance to the camera
    //float fogFactor = clamp((distance - fogStart) / (fogEnd - fogStart), 0.0, 1.0);

    float fogFactor = exp(-distance * fogDensity);
    fogFactor = clamp(fogFactor, 0.0, 1.0);


    // Blend the lit color with the fog color
    //finalColor = mix(fogColor, glowCol, fogFactor);

    //finalColor = vec3(distance / 500.0); // Normalize distance for visualization

    //finalColor = vec3(fogFactor); // Visualize fog factor as grayscale

    
    //finalColor = vec3(1.0, 0.0, 0.0);

    

    //finalColor = worldPosition;
}
