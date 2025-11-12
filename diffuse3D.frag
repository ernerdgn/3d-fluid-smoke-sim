#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

// uniforms
uniform sampler3D u_x;
uniform sampler3D u_b;
uniform int   u_zSlice;
uniform float u_alpha;
uniform float u_rBeta;

void main()
{
    ivec3 coord = ivec3(gl_FragCoord.xy, u_zSlice);
    
    vec4 b_val = texelFetch(u_b, coord, 0); // 0 = LOD
    
    vec4 x_left   = texelFetch(u_x, coord + ivec3(-1,  0,  0), 0);
    vec4 x_right  = texelFetch(u_x, coord + ivec3( 1,  0,  0), 0);
    vec4 x_down   = texelFetch(u_x, coord + ivec3( 0, -1,  0), 0);
    vec4 x_up     = texelFetch(u_x, coord + ivec3( 0,  1,  0), 0);
    vec4 x_back   = texelFetch(u_x, coord + ivec3( 0,  0, -1), 0);
    vec4 x_front  = texelFetch(u_x, coord + ivec3( 0,  0,  1), 0);
    
    vec4 neighbor_sum = x_left + x_right + x_down + x_up + x_back + x_front;
    
    // x_new = (b_val + a * neighbor_sum) / (1 + 6*a)
    FragColor = (b_val + u_alpha * neighbor_sum) * u_rBeta;
}