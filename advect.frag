#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

// uniforms
uniform sampler2D u_velocity_field;
uniform sampler2D u_quantity_to_move;
uniform float u_dt;  // delta time
uniform vec2 u_grid_size;

// 1-- get velo at pixel
// 2-- trace back in time
// 3-- sample quantity from 2
// 4-- use 3

void main()
{
	vec2 vel = texture(u_velocity_field, TexCoords).xy;

	vec2 pixelCoords = TexCoords * u_grid_size;
	vec2 prevCoords = pixelCoords - vel * u_dt; // pervcoords xdxd
	vec2 normalizedPrevCoords = prevCoords / u_grid_size;

	vec4 newQuantity = texture(u_quantity_to_move, normalizedPrevCoords);

	FragColor = newQuantity;
}