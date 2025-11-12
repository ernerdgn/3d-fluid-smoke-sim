#version 430 core
out vec4 FragColor;

in vec3 v_worldPos;
in vec3 v_texCoords;

// uniforms
uniform sampler3D u_volume_texture;
uniform vec3 u_camera_pos;
uniform mat4 u_model_inv;

void main()
{
    // get ray dir from world space
    vec3 ray_dir_world = normalize(v_worldPos - u_camera_pos);

    // transform ray from world to model
    vec3 ray_dir_model = (u_model_inv * vec4(ray_dir_world, 0.0)).xyz;
    
    // RAY MARCHING
    
    // ray starts from back
    vec3 ray_pos = v_texCoords; 
    
    // step towards to cam
    vec3 ray_step = normalize(ray_dir_model) * .01;
    
    vec4 accumulated_color = vec4(.0);
    int num_steps = 64;

    for (int i = 0; i < num_steps; i++)
    {
        // sample density
        float density = texture(u_volume_texture, ray_pos).r;
        //float display_density = density * .1;

        if (density > .01)
        {
            float constant_alpha = .05;

            vec3 color = vec3(1.0, 1.0, 1.0) * density;

            accumulated_color.rgb = (color * constant_alpha) + (accumulated_color.rgb * (1.0 - constant_alpha));
            accumulated_color.a = constant_alpha + (accumulated_color.a * (1.0 - constant_alpha));
        }
        
        // move ray
        ray_pos += ray_step; // march forward
        
        // stop if exit or opaque
        if (accumulated_color.a > 0.95 || 
            any(lessThan(ray_pos, vec3(0.0))) || 
            any(greaterThan(ray_pos, vec3(1.0))))
        {
            break;
        }
    }
    
    FragColor = accumulated_color;
}