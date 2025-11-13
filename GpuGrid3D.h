#pragma once
#include <glad/glad.h>
#include "shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class GpuGrid3D
{
public:
	GpuGrid3D(int width, int height, int depth);
	~GpuGrid3D();

	void step(Shader& splatShader, 
		const glm::vec3& mouse_pos3D, const glm::vec3& mouse_vel,
		bool is_bouncing);

	void clear(Shader& clearComputeShader);

	void swapDensityBuffers();
	void swapVelocityBuffers();

	GLuint getDensityTexture() { return m_densityTexA; }
	GLuint getVelocityTexture() { return m_velocityTexA; }

public:
	int m_width, m_height, m_depth;

	// ping-pong buffers (single R comp)
	GLuint m_densityTexA, m_densityTexB;

	// ping-pong buffers (3 comp: r,g,b)
	GLuint m_velocityTexA, m_velocityTexB;

private:
	GLuint create3DTexture(int internal_format, int format);

	void getWorkGroups(GLuint& groupsX, GLuint& groupsY, GLuint& groupsZ);
};