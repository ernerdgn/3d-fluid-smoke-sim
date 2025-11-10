#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D u_texture;

void main()
{
	float density = texture(u_texture, TexCoords).r;  // extract red channel
	
	// draw density as grayscale
	FragColor = vec4(vec3(density), 1.0);
}