#pragma once
#include <glad/glad.h>

class GpuGrid
{
public:
	GpuGrid(int width, int height);
	~GpuGrid();

	void swapVelocityBuffers();
	void swapDensityBuffers();

	// getters
	GLuint getDensityTexture() { return m_densityTexA; }
	GLuint getVelocityTexture() { return m_velocityTexA; }
	GLuint getPressureTexture() { return m_pressureTexA; }

public:
	// buffers for velocity, x,y
	GLuint m_velocityFboA, m_velocityFboB;
	GLuint m_velocityTexA, m_velocityTexB;

	// buffers for dens, store single val
	GLuint m_densityFboA, m_densityFboB;
	GLuint m_densityTexA, m_densityTexB;

	// util buffers of project
	GLuint m_divergenceFbo, m_divergenceTex;
	GLuint m_pressureFboA, m_pressureFboB;
	GLuint m_pressureTexA, m_pressureTexB;

private:
	int m_width, m_height;

	void createFbo(GLuint& fbo, GLuint& texture, int internal_format, int format);
};