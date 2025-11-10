#include "FluidGrid.h"

static int IX(int x, int y, int width)
{
	// clamp
	if (x < 0) x = 0;
	if (x >= width) x = width - 1;
	if (y < 0) y = 0;
	if (y >= width) y = width - 1;  // its a square, so its valid

	return x + y * width;
}

static float lerp(float a, float b, float t) // <3
{
	return a + t * (b - a);
}

static float sample(const std::vector<float>&buffer, float x, float y, int width)
{
	int x0 = (int)x;
	int x1 = x0 + 1;
	int y0 = (int)y;
	int y1 = y0 + 1;

	// fractional part
	float sx = x - (float)x0;
	float sy = y - (float)y0;

	// sample corners
	float v00 = buffer[IX(x0, y0, width)];
	float v10 = buffer[IX(x1, y0, width)];
	float v01 = buffer[IX(x0, y1, width)];
	float v11 = buffer[IX(x1, y1, width)];

	// bilinear interp
	float top = lerp(v00, v10, sx);
	float bottom = lerp(v01, v11, sx);

	return lerp(top, bottom, sy);
}

static glm::vec2 sampleVec2(const std::vector<glm::vec2>& buffer, float x, float y, int width)
{
	int x0 = (int)x;
	int x1 = x0 + 1;
	int y0 = (int)y;
	int y1 = y0 + 1;
	// fractional part
	float sx = x - (float)x0;
	float sy = y - (float)y0;
	// sample corners
	glm::vec2 v00 = buffer[IX(x0, y0, width)];
	glm::vec2 v10 = buffer[IX(x1, y0, width)];
	glm::vec2 v01 = buffer[IX(x0, y1, width)];
	glm::vec2 v11 = buffer[IX(x1, y1, width)];
	// bilinear interp
	glm::vec2 top = glm::mix(v00, v10, sx);
	glm::vec2 bottom = glm::mix(v01, v11, sx);
	return glm::mix(top, bottom, sy);
}

FluidGrid::FluidGrid(int width, int height)
	: m_width(width), m_height(height), m_delta_time(.1f), m_viscosity(.0001f)
{
	int size = width * height;
	m_density_read.resize(size, 0.0f);
	m_density_write.resize(size, 0.0f);
	m_velocity_read.resize(size, glm::vec2(0.0f));
	m_velocity_write.resize(size, glm::vec2(0.0f));

	m_divergence.resize(size, 0.0f);
	m_pressure.resize(size, 0.0f);
}

void FluidGrid::addDensity(int x, int y, float amount)
{
	m_density_read[IX(x, y, m_width)] += amount;
}

void FluidGrid::addVelocity(int x, int y, float forceX, float forceY)
{
	m_velocity_read[IX(x, y, m_width)] += glm::vec2(forceX, forceY);
}

void FluidGrid::swapBuffers()
{
	std::swap(m_density_read, m_density_write);
	std::swap(m_velocity_read, m_velocity_write);
}

void FluidGrid::advect(std::vector<float>& read_buffer, std::vector<float>& write_buffer, const std::vector<glm::vec2>& velocity_field)
{
	for (int y = 0; y < m_height; ++y)
	{
		for (int x = 0; x < m_width; ++x)
		{
			// get 1d index
			int index = IX(x, y, m_width);

			// get velocity
			glm::vec2 velocity = velocity_field[index];

			// backtrace
			float current_x = (float)x + .5f;
			float current_y = (float)y + .5f;
			// back to the future
			float prev_x = current_x - velocity.x * m_delta_time;
			float prev_y = current_y - velocity.y * m_delta_time;

			// sample
			float new_density = sample(read_buffer, prev_x, prev_y, m_width);

			// write
			write_buffer[index] = new_density;
		}
	}
}

void FluidGrid::diffuse(std::vector<float>& read_buffer, std::vector<float>& write_buffer, float diff_rate)
{
	int iter = 20;

	// constant related to diffusion rate and time step
	float a = m_delta_time * diff_rate * m_width * m_height;  // can be width*width for square grid

	// Gauss-Seidel relaxation
	std::vector<float>& v = write_buffer; // solving for this
	const std::vector<float>& v0 = read_buffer;  // initial state

	for (int k = 0; k < iter; ++k)
	{
		for (int y = 0; y < m_height; ++y)
		{
			for (int x = 0; x < m_width; ++x)
			{
				int index = IX(x, y, m_width);

				// get vals of neighboring cells
				float neighbor_sum = 
					v[IX(x - 1, y, m_width)] +  // left
					v[IX(x + 1, y, m_width)] +  // rigth
					v[IX(x, y - 1, m_width)] +  // bottom
					v[IX(x, y + 1, m_width)];   // top

				v[index] = (v0[index] + a * neighbor_sum) / (1 + 4 * a);
			}
		}
	}
}

void FluidGrid::diffuseVelocity(const std::vector<glm::vec2>& read_buffer, std::vector<glm::vec2>& write_buffer, float diff_rate)
{
	int iter = 20;

	// constant related to diffusion rate and time step
	float a = m_delta_time * diff_rate * m_width * m_height;  // can be width*width for square grid
	
	// Gauss-Seidel relaxation
	std::vector<glm::vec2>& v = write_buffer; // solving for this
	const std::vector<glm::vec2>& v0 = read_buffer;  // initial state
	
	for (int k = 0; k < iter; ++k)
	{
		for (int y = 0; y < m_height; ++y)
		{
			for (int x = 0; x < m_width; ++x)
			{
				int index = IX(x, y, m_width);
				
				// get vals of neighboring cells
				glm::vec2 neighbor_sum =
					v[IX(x - 1, y, m_width)] +  // left
					v[IX(x + 1, y, m_width)] +  // rigth
					v[IX(x, y - 1, m_width)] +  // bottom
					v[IX(x, y + 1, m_width)];   // top
				
				v[index] = (v0[index] + a * neighbor_sum) / (1.0f + 4.0f * a);
			}
		}
	}
}

void FluidGrid::advectVelocity(const std::vector<glm::vec2>& read_buffer, std::vector<glm::vec2>& write_buffer, const std::vector<glm::vec2>& velocity_field)
{
	for (int y = 0; y < m_height; ++y)
	{
		for (int x = 0; x < m_width; ++x)
		{
			// get 1d index
			int index = IX(x, y, m_width);
			// get velocity
			glm::vec2 velocity = velocity_field[index];
			// backtrace
			float current_x = (float)x + .5f;
			float current_y = (float)y + .5f;
			// back to the future
			float prev_x = current_x - velocity.x * m_delta_time;
			float prev_y = current_y - velocity.y * m_delta_time;
			// write
			write_buffer[index] = sampleVec2(read_buffer, prev_x, prev_y, m_width);
		}
	}
}

void FluidGrid::project(std::vector<glm::vec2>& velocity_field)
{
	// compute divergence
	float h = 1.0f / m_width; // assuming square grid
	for (int y = 0; y < m_height; ++y)
	{
		for (int x = 0; x < m_width; ++x)
		{
			int index = IX(x, y, m_width);
			float div =
				(velocity_field[IX(x + 1, y, m_width)].x - velocity_field[IX(x - 1, y, m_width)].x +
				 velocity_field[IX(x, y + 1, m_width)].y - velocity_field[IX(x, y - 1, m_width)].y);
			m_divergence[index] = -.5f * h * div;
			m_pressure[index] = .0f; // reset pressure, ohhh
		}
	}

	// solve for pressure using Gauss-Seidel
	int iter = 20;
	for (int k = 0; k < iter; ++k)
	{
		for (int y = 0; y < m_height; ++y)
		{
			for (int x = 0; x < m_width; ++x)
			{
				int index = IX(x, y, m_width);
				float neighbor_sum =
					m_pressure[IX(x - 1, y, m_width)] +
					m_pressure[IX(x + 1, y, m_width)] +
					m_pressure[IX(x, y - 1, m_width)] +
					m_pressure[IX(x, y + 1, m_width)];
				m_pressure[index] = (m_divergence[index] + neighbor_sum) / 4.0f;
			}
		}
	}

	// subtract pressure gradient from velocity field
	float inv_h = .5f / h;  // .5f*m_width
	for (int y = 0; y < m_height; ++y)
	{
		for (int x = 0; x < m_width; ++x)
		{
			int index = IX(x, y, m_width);
			float p_right = m_pressure[IX(x + 1, y, m_width)];
			float p_left = m_pressure[IX(x - 1, y, m_width)];
			float p_top = m_pressure[IX(x, y + 1, m_width)];
			float p_bottom = m_pressure[IX(x, y - 1, m_width)];
			
			velocity_field[index].x -= inv_h * (p_right - p_left);
			velocity_field[index].y -= inv_h * (p_top - p_bottom);
		}
	}
}

void FluidGrid::step()
{
	// m_read = N, m_write = N-1, ping-pong buffers

	// diff velocity
	diffuseVelocity(m_velocity_read, m_velocity_write, m_viscosity);
	// res-> m_write = diffused velocity (N)

	// diff density
	diffuse(m_density_read, m_density_write, .00001f);
	// res-> m_write = diffused density (N)

	// swap aq
	swapBuffers();
	// res-> m_read = diffused N, m_write = N

	// project velocity
	project(m_velocity_read);
	// res-> m_read = projected( diffused (N-1 + input)), m_write = N-1

	// advect density
	advect(m_density_read, m_density_write, m_velocity_read);

	// advect velocity
	advectVelocity(m_velocity_read, m_velocity_write, m_velocity_read);

	// swap aq
	swapBuffers();
}
