#include "GpuGrid.h"
#include <iostream>

GpuGrid::GpuGrid(int width, int height) : m_width(width), m_height(height)
{
	// velo, red-green channels
	createFbo(m_velocityFboA, m_velocityTexA, GL_RG32F, GL_RG);
	createFbo(m_velocityFboB, m_velocityTexB, GL_RG32F, GL_RG);

	// dens, red channel
	createFbo(m_densityFboA, m_densityTexA, GL_R32F, GL_RED);
	createFbo(m_densityFboB, m_densityTexB, GL_R32F, GL_RED);

	// divergence, red
	createFbo(m_divergenceFbo, m_divergenceTex, GL_R32F, GL_RED);

	// pressure, red
	createFbo(m_pressureFboA, m_pressureTexA, GL_R32F, GL_RED);
	createFbo(m_pressureFboB, m_pressureTexB, GL_R32F, GL_RED);
}

GpuGrid::~GpuGrid()
{
	// todo
}

void GpuGrid::swapVelocityBuffers()
{
	std::swap(m_velocityFboA, m_velocityFboB);
	std::swap(m_velocityTexA, m_velocityTexB);
}

void GpuGrid::createFbo(GLuint& fbo, GLuint& texture, int internal_format, int format)
{
	// create tex
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// set params
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// create tex with null
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, m_width, m_height, 0, format, GL_FLOAT, nullptr);
	
	// fbo
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// attach tex to fbo
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

	// safety check
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR::FRAMEBUFFER:: framebuffer is shit" << std::endl;
	}

	// unbind
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GpuGrid::swapDensityBuffers()
{
	std::swap(m_densityFboA, m_densityFboB);
	std::swap(m_densityTexA, m_densityTexB);
}