#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

// uniforms
uniform sampler2D u_velocity_field;
uniform sampler2D u_curl_field;

uniform float u_epsilon; // effect power
uniform float u_dt;

void main()
{
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // neighbor curls
    float curl_left  = texelFetch(u_curl_field, coord + ivec2(-1, 0), 0).r;
    float curl_right = texelFetch(u_curl_field, coord + ivec2(1, 0), 0).r;
    float curl_up    = texelFetch(u_curl_field, coord + ivec2(0, 1), 0).r;
    float curl_down  = texelFetch(u_curl_field, coord + ivec2(0, -1), 0).r;
    float curl_center= texelFetch(u_curl_field, coord, 0).r;

    // direction of curl
    vec2 grad = vec2(curl_right - curl_left, curl_up - curl_down);
    
    // normalize, .00001 is added to prevent div by 0
    vec2 N = normalize(grad + vec2(1e-5)); 
    
    // confinement force, perpendicular to N
    vec2 force = u_epsilon * curl_center * vec2(N.y, -N.x);

    // add
    vec2 vel = texelFetch(u_velocity_field, coord, 0).xy;
    vel += force * u_dt;

    FragColor = vec4(vel, 0.0, 1.0);
}