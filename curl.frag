#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

// uniforms
uniform sampler2D u_velocity_field;

void main()
{
	ivec2 coord = ivec2(gl_FragCoord.xy);

    // neigbor velos
	float v_left  = texelFetch(u_velocity_field, coord + ivec2(-1, 0), 0).y;
    float v_right = texelFetch(u_velocity_field, coord + ivec2(1, 0), 0).y;
    float u_up    = texelFetch(u_velocity_field, coord + ivec2(0, 1), 0).x;
    float u_down  = texelFetch(u_velocity_field, coord + ivec2(0, -1), 0).x;

    // (dv/dx - du/dy)
    float curl = (v_right - v_left) - (u_up - u_down);

    // a little scale down doesnt hurt
    FragColor = vec4(curl * .5, .0, .0, 1.0);
}