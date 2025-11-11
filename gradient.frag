#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

// uniforms
uniform sampler2D u_velocity_field;
uniform sampler2D u_pressure_field;
uniform vec2 u_grid_size;

void main()
{
	ivec2 coord = ivec2(gl_FragCoord.xy);
    float h = 1.0 / u_grid_size.x;
    float inv_h = .5 / h;

    // get neighbor pressure vals
    float p_right = texelFetch(u_pressure_field, coord + ivec2(1, 0), 0).r;
    float p_left  = texelFetch(u_pressure_field, coord + ivec2(-1, 0), 0).r;
    float p_up    = texelFetch(u_pressure_field, coord + ivec2(0, 1), 0).r;
    float p_down  = texelFetch(u_pressure_field, coord + ivec2(0, -1), 0).r;

    // current diffed vel
    vec2 vel = texelFetch(u_velocity_field, coord, 0).xy;

    // subs press gradient
    vel.x -= inv_h * (p_right - p_left);
    vel.y -= inv_h * (p_up - p_down);

    FragColor = vec4(vel, 0.0, 1.0);
}