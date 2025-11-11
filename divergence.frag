#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

// uniforms
uniform sampler2D u_velocity_field;
uniform vec2 u_grid_size;

void main()
{
	ivec2 coord = ivec2(gl_FragCoord.xy);
	float h = 1.0 / u_grid_size.x;

	// get neigbor velos
	float vel_right = texelFetch(u_velocity_field, coord + ivec2(1, 0), 0).x;
	float vel_left = texelFetch(u_velocity_field, coord + ivec2(-1, 0), 0).x;
	float vel_up = texelFetch(u_velocity_field, coord + ivec2(0, 1), 0).y;
	float vel_down = texelFetch(u_velocity_field, coord + ivec2(0, -1), 0).y;

	// compute div
	float divergence = -.5 * h * (vel_right - vel_left + vel_up - vel_down);

	FragColor = vec4(divergence, .0, .0, 1.0);
}