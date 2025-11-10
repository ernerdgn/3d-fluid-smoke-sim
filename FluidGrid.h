#pragma once
#include <vector>
#include <glm/glm.hpp>

class FluidGrid
{
public:
	FluidGrid(int width, int height);

	void step();

	void addDensity(int x, int y, float amount);
	void addVelocity(int x, int y, float forceX, float forceY);

	const std::vector<float>& getDensity() { return m_density_read; }
	const std::vector<glm::vec2>& getVelocity() { return m_velocity_read; }
	const std::vector<float>& getPressure() { return m_pressure; }

private:
	int m_width;
	int m_height;
	float m_delta_time;
	float m_viscosity;

	void swapBuffers();
	void advect(std::vector<float>& read_buffer, std::vector<float>& write_buffer, const std::vector<glm::vec2>& velocity_field);
	void diffuse(std::vector<float>& read_buffer, std::vector<float>& write_buffer, float diff_rate);
	void diffuseVelocity(const std::vector<glm::vec2>& read_buffer, std::vector<glm::vec2>& write_buffer, float diff_rate);
	void advectVelocity(const std::vector<glm::vec2>& read_buffer, std::vector<glm::vec2>& write_buffer, const std::vector<glm::vec2>& velocity_field);
	void project(std::vector<glm::vec2>& velocity_field);

	// sim data
	std::vector<float> m_density_read;
	std::vector<float> m_density_write;
	std::vector<glm::vec2> m_velocity_read;
	std::vector<glm::vec2> m_velocity_write;

	// projection buffers
	std::vector<float> m_divergence;
	std::vector<float> m_pressure;
};