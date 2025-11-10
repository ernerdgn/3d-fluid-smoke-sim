#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D u_densityTex;
uniform sampler2D u_velocityTex;
uniform sampler2D u_pressureTex;

uniform int u_debugMode; // 0: density, 1: velocity, 2: pressure

vec4 visualizeVelocity(vec2 vel)
{
	float r = vel.x * .5 + .5; // map from [-1,1] to [0,1]
	float g = vel.y * .5 + .5; // map from [-1,1] to [0,1]
	return vec4(r, g, 0.0, 1.0);
}

vec4 visualizePressure(float p)
{
	vec3 color = vec3(.0);
	if (p > .0)
	{
		color.r = p; // positive pressure in red
	}
	else
	{
		color.b = -p; // negative pressure in blue
	}

	return vec4(clamp(color, .0, 1.0), 1.0);
}

void main()
{
	if (u_debugMode == 0)
	{
		float density = texture(u_densityTex, TexCoords).r;
		FragColor = vec4(vec3(density), 1.0);
	}
	else if (u_debugMode == 1)
	{
		vec2 velocity = texture(u_velocityTex, TexCoords).rg;
		FragColor = visualizeVelocity(velocity * .1);
	}
	else if (u_debugMode == 2)
	{
		float pressure = texture(u_pressureTex, TexCoords).r;
		FragColor = visualizePressure(pressure * .01);
	}
	else
	{
		FragColor = vec4(1.0, 0.0, 1.0, 1.0); // magenta for invalid mode
	}
}