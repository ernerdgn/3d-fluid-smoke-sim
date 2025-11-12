#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

// uniforms
uniform sampler3D u_read_texture; 
uniform int u_zSlice;
uniform vec3 u_brush_center3D; 
uniform float u_radius;
uniform int u_is_bouncing;
uniform vec3 u_force; 

void main()
{
    // get coords of pixel
    ivec3 texel_coord = ivec3(gl_FragCoord.xy, u_zSlice);
    
    // read old val
    vec4 read_val = texelFetch(u_read_texture, texel_coord, 0); // 0->LOD

    // compute 3d dist
    float dist = distance(vec3(texel_coord), u_brush_center3D);
    
    // compute splat strength
    float splat = exp(-dist / u_radius) * float(u_is_bouncing);
    
    // add force
    FragColor = read_val + vec4(u_force, 0.0) * splat;
}