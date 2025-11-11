#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

// uniforms
uniform sampler2D u_read_texture;
uniform vec2 u_mouse_pos;
uniform float u_radius; // brush rad
uniform vec3 u_force;
uniform int u_is_bouncing;

void main()
{
	vec4 read_val = texture(u_read_texture, TexCoords);

	// dist between pixel and mouse
	float dist = distance(gl_FragCoord.xy, u_mouse_pos);

	float splat = exp(-dist / u_radius) * float(u_is_bouncing);

	FragColor = read_val + vec4(u_force, 1.0) * splat;
	FragColor.a = 1.0;
}