#version 430 core
layout (location = 0) in vec3 aPos;

// uniforms, cam matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// out
out vec3 v_worldPos;
out vec3 v_texCoords;

void main()
{
	// pass the vertex position
    v_worldPos = (model * vec4(aPos, 1.0)).xyz;
    
    // pass 3d textCoord
    v_texCoords = aPos + 0.5;
    
    // default 3d transform
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
