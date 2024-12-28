#version 330 core

in vec3 color;

// TODO: To add UV input to this fragment shader 
in vec2 uv;

uniform sampler2D textureSampler;

// TODO: To add the texture sampler

out vec3 finalColor;

void main()
{
	vec3 textureColor = texture(textureSampler, uv).rgb;

	finalColor = textureColor;
	//finalColor = vec3(uv, 0.0);

	// TODO: texture lookup. 

}
