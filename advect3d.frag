#version 430 core
out vec4 FragColor;
in vec2 TexCoords;  // gl_FragCoord

// uniforms
uniform sampler3D u_velocity_field;
uniform sampler3D u_quantity_to_move;
uniform int u_zSlice;
uniform vec3 u_grid_size;
uniform float u_dt;

void main()
{
	// get coords from texel
	ivec3 texel_coord = ivec3(gl_FragCoord.xy, u_zSlice);

	// get velo
	vec3 vel = texelFetch(u_velocity_field, texel_coord, 0).xyz;

	// trace back
	vec3 current_pos = vec3(texel_coord);

	// get prev pos
	vec3 prev_pos = current_pos;

	// sample quantity
	vec3 normalized_prev_pos = prev_pos / u_grid_size;

	// texture() does lerp, yey!
	vec4 new_quantity = texture(u_quantity_to_move, normalized_prev_pos);

	FragColor = new_quantity;
}