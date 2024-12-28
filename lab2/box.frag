#version 330 core

in vec3 color;
in vec3 FragPos;

// TODO: To add UV input to this fragment shader 
in vec2 uv;

uniform sampler2D textureSampler;

// TODO: To add the texture sampler
out vec3 finalColor; // WORKS


void main()
{
	vec3 textureColor = texture(textureSampler, uv).rgb; // WORKS
	
	finalColor = textureColor; // WORKS

}
