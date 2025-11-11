#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

// uniforms
uniform sampler2D u_pressure;
uniform sampler2D u_divergence;

void main()
{
	ivec2 coord = ivec2(gl_FragCoord.xy);

	// get div
    vec4 b_val = texelFetch(u_divergence, coord, 0);

    // get neighbor pressure vals
    vec4 p_left   = texelFetch(u_pressure, coord + ivec2(-1,  0), 0);
    vec4 p_right  = texelFetch(u_pressure, coord + ivec2( 1,  0), 0);
    vec4 p_down   = texelFetch(u_pressure, coord + ivec2( 0, -1), 0);
    vec4 p_up     = texelFetch(u_pressure, coord + ivec2( 0,  1), 0);
    
    vec4 neighbor_sum = p_left + p_right + p_down + p_up;

    FragColor = (b_val + neighbor_sum) * .25;
}