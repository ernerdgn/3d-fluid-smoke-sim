#version 430 core
out vec4 FragColor;

in vec3 v_texCoords;

uniform sampler3D u_volume_texture;

void main()
{
    float density = texture(u_volume_texture, v_texCoords).r;
    
    if (density < 0.1) {
        discard;
    }
    
    FragColor = vec4(density, density, density, 1.0);
}