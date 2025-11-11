#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

// uniforms
uniform sampler2D u_x; // read buffer
uniform sampler2D u_b; // advected data
uniform float u_alpha;
uniform float u_rBeta; // 1 / (1+4*a)

// 1-- get origin val from adv
// 2-- get neighbor vals from last iter
// 3-- compute new val

void main()
{
	ivec2 coord = ivec2(gl_FragCoord.xy);

	vec4 b_val = texelFetch(u_b, coord, 0);  // 0 -> LOD

	vec4 x_left = texelFetch(u_x, coord + ivec2(-1, 0), 0);
	vec4 x_right = texelFetch(u_x, coord + ivec2(1, 0), 0);
	vec4 x_down = texelFetch(u_x, coord + ivec2(0, -1), 0);
	vec4 x_up = texelFetch(u_x, coord + ivec2(0, 1), 0);

	vec4 neighbor_sum = x_left + x_right + x_down + x_up;

	FragColor = (b_val + u_alpha * neighbor_sum) * u_rBeta;
}