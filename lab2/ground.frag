#version 330 core

in vec3 color;
in vec3 worldPosition;
in vec3 worldNormal; 


// TODO: To add UV input to this fragment shader 
in vec2 uv;

uniform sampler2D textureSampler;

// Sphere light uniforms
uniform vec3 sphereLightPos;      
uniform vec3 sphereLightColor;    
uniform float sphereLightIntensity;

uniform vec3 viewPos;

out vec3 finalColor;

void main()
{
	 // Fetch texture color
    vec3 textureColor = texture(textureSampler, uv).rgb;

    // Normalize the normal vector
    vec3 norm = normalize(worldNormal); // Use worldNormal directly

    // Calculate direction to the sphere light
    vec3 lightDir = normalize(sphereLightPos - worldPosition); // Use worldPosition directly

    // Diffuse lighting
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = textureColor * sphereLightColor * diff * sphereLightIntensity;

    // Specular lighting
    vec3 viewDir = normalize(viewPos - worldPosition);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); // Shininess factor is 32.0
    vec3 specular = vec3(1.0) * spec * sphereLightColor * sphereLightIntensity;

    // Light attenuation
    float distance = length(sphereLightPos - worldPosition);
    float attenuation = 1.0 / (1.0 + 0.01 * distance + 0.001 * (distance * distance));
	//float attenuation = 1.0 / (1.0 + distance);
	//float attenuation = 1.0;
    diffuse *= attenuation;
    specular *= attenuation;

    // Combine texture color with lighting
    finalColor = diffuse + specular; // BLACK 
	//finalColor = diffuse; // BLACK
	//finalColor = specular; // BLACK

	//finalColor = texture(textureSampler, uv).rgb;

	//finalColor = vec3(distance / 50.0); // GRADIENT CORRECT
	//finalColor = vec3(attenuation); // PURE WHITE
	//finalColor = vec3(1.0 / (1.0 + 0.01 * distance)); // OPPOSITE GRADIENT CORRECT

	//finalColor = (worldPosition * 0.01) + 0.5; // GRADIENT CORRECT

 	//finalColor = vec3(worldNormal.x + 0.5, worldNormal.y, worldNormal.z + 0.5);

	//finalColor = vec3((worldNormal * 0.05) + 0.5);  // PURE BLACK
	//finalColor = worldNormal;
	//finalColor = vec3(worldNormal.x * 0.5 + 0.5, 0.0, 0.0); // PURE BLACK
	//finalColor = vec3(uv, 0.0); // PURE YELLOW
	//finalColor = vec3(worldNormal.y); // PURE BLACK
	//finalColor = (norm * 0.5) + 0.5; // BLACK
	//finalColor = (lightDir * 0.5) + 0.5; // GRADIENT AROUND LIGHT SOURCE
	//finalColor = vec3(diff); // BLACK
	//finalColor = vec3(spec); // BLACK
	//finalColor = (reflectDir * 0.5) + 0.5; // BLACK
	//finalColor = sphereLightColor * sphereLightIntensity; // WHITE

	//finalColor = textureColor; // WORKING

}


