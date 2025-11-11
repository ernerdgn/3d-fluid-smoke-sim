#include <iostream>
#include <glad/glad.h>
#include <glfw3.h>
#include "shader.h"
//#include "FluidGrid.h"
#include "GpuGrid.h"
#include <glm/gtc/type_ptr.hpp>

//float quad_vertices[] = {
//	// positions      // tex coords
//	-.5f,  .5f,     0.0f, .5f,
//	-.5f, -.5f,     0.0f, 0.0f,
//	 .5f, -.5f,     .5f, 0.0f,
//
//	-.5f,  .5f,     0.0f, .5f,
//	 .5f, -.5f,     .5f, 0.0f,
//	 .5f,  .5f,     .5f, .5f
//};

float quad_vertices[] =
{
	// positions   // tex coords
	-1.0f,  1.0f,   0.0f, 1.0f,
	-1.0f, -1.0f,   0.0f, 0.0f,
	 1.0f, -1.0f,   1.0f, 0.0f,

	-1.0f,  1.0f,   0.0f, 1.0f,
	 1.0f, -1.0f,   1.0f, 0.0f,
	 1.0f,  1.0f,   1.0f, 1.0f
};

// grid size
const int GRID_WIDTH = 512;
const int GRID_HEIGHT = 512;

int g_DebugMode = 0; // 0: density, 1: velocity, 2: pressure

int main()
{
	// glfw init
	if (!glfwInit())
	{
		std::cerr << "failed to initialize GLFW" << std::endl;
		return -1;
	}

	// set OpenGL version to 3.4 core profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// create window
	GLFWwindow* window = glfwCreateWindow(800, 600, "fluild sim", nullptr, nullptr);

	if (!window)
	{
		std::cerr << "failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// make context current
	glfwMakeContextCurrent(window);

	// glad init
	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "failed to initialize GLAD" << std::endl;
		return -1;
	}

	// viewport
	//glViewport(0, 0, 800, 600);

	glfwSetWindowSize(window, GRID_WIDTH, GRID_HEIGHT);
	glViewport(0, 0, GRID_WIDTH, GRID_HEIGHT);


	// setup quad vao vbo
	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	// bind vao
	glBindVertexArray(VAO);

	// bind and set vbo
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// load shaders
	Shader quadShader("quad.vert", "quad.frag");
	Shader splatShader("quad.vert", "splat.frag");
	Shader clearShader("quad.vert", "clear.frag");
	Shader advectShader("quad.vert", "advect.frag");
	Shader diffuseShader("quad.vert", "diffuse.frag");
	Shader divergenceShader("quad.vert", "divergence.frag");
	Shader pressureShader("quad.vert", "pressure.frag");
	Shader gradientShader("quad.vert", "gradient.frag");

	glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (action == GLFW_PRESS)
		{
			if (key == GLFW_KEY_ESCAPE)
				glfwSetWindowShouldClose(window, true);

			else if (key == GLFW_KEY_D)
			{
				std::cout << "dispMode density" << std::endl;
				g_DebugMode = 0;
			}
			else if (key == GLFW_KEY_V)
			{
				std::cout << "dispMode velocity" << std::endl;
				g_DebugMode = 1;
			}
			else if (key == GLFW_KEY_P)
			{
				std::cout << "dispMode pressure" << std::endl;
				g_DebugMode = 2;
			}
		}
	});

	// mouse interaction
	struct MouseState
	{
		bool pressed = false;
		double x = 0.0;
		double y = 0.0;
	} mouse;

	glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods)
		{
			MouseState* mouse = (MouseState*)glfwGetWindowUserPointer(window);
			if (button == GLFW_MOUSE_BUTTON_LEFT)
			{
				if (action == GLFW_PRESS)
					mouse->pressed = true;
				else if (action == GLFW_RELEASE)
					mouse->pressed = false;
			}
		});

	glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos)
		{
			MouseState* mouse = (MouseState*)glfwGetWindowUserPointer(window);
			mouse->x = xpos;
			mouse->y = ypos;
		});

	glfwSetWindowUserPointer(window, &mouse);

	/* CPU GRID */
	//FluidGrid fluidGrid(GRID_WIDTH, GRID_HEIGHT);

	//// texture
	//unsigned int display_texture;
	//glGenTextures(1, &display_texture);
	//glBindTexture(GL_TEXTURE_2D, display_texture);

	//// tex params
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//// allocate texture on gpu
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, GRID_WIDTH, GRID_HEIGHT, 0, GL_RED, GL_FLOAT, nullptr);
	//glBindTexture(GL_TEXTURE_2D, 0); // unbind

	//// velocity and pressure textures (for debug display)
	//// velocity
	//unsigned int velocity_texture;
	//glGenTextures(1, &velocity_texture);
	//glBindTexture(GL_TEXTURE_2D, velocity_texture);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//// red+green for velocity
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, GRID_WIDTH, GRID_HEIGHT, 0, GL_RG, GL_FLOAT, NULL);

	//// pressure
	//unsigned int pressure_texture;
	//glGenTextures(1, &pressure_texture);
	//glBindTexture(GL_TEXTURE_2D, pressure_texture);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//// red for pressure
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, GRID_WIDTH, GRID_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);

	//glBindTexture(GL_TEXTURE_2D, 0); // unbind

	/* GPU GRID */
	GpuGrid gpuGrid(GRID_WIDTH, GRID_HEIGHT);

	// clear fbos to 0
	glViewport(0, 0, GRID_WIDTH, GRID_HEIGHT);
	clearShader.use();

	// list of fbos
	GLuint fbos[] = {
		gpuGrid.m_velocityFboA, gpuGrid.m_velocityFboB,
		gpuGrid.m_densityFboA, gpuGrid.m_densityFboB,
		gpuGrid.m_divergenceFbo,
		gpuGrid.m_pressureFboA, gpuGrid.m_pressureFboB
	};

	for (GLuint fbo : fbos)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glDrawArrays(GL_TRIANGLES, 0, 6); // clear quad
	}

	glm::vec2 lastMousePos = glm::vec2(.0f);

	// constants
	// brush radius
	float brush_radius = GRID_WIDTH * .0075f;  // .75% of the grid
	// time
	float time_step = .1f;
	// viscosity
	float viscosity = .0001f;
	// diffuse iters
	int diffuse_iterations = 20;
	// pressure iters
	int pressure_iterations = 40;


	// main loop
	while (!glfwWindowShouldClose(window))
	{
		// poll events
		glfwPollEvents();

		// sim
		// splatSHADER
		glViewport(0, 0, GRID_WIDTH, GRID_HEIGHT);
		splatShader.use();

		// compute mouse vel
		glm::vec2 mousePos = glm::vec2(mouse.x, GRID_HEIGHT - mouse.y);
		glm::vec2 mouseVel = mousePos - lastMousePos;
		lastMousePos = mousePos;

		// set uniforms for the splat shader
		glUniform2fv(glGetUniformLocation(splatShader.ID, "u_mouse_pos"), 1, glm::value_ptr(mousePos));
		glUniform1f(glGetUniformLocation(splatShader.ID, "u_radius"), brush_radius); // 1.5% of the grid
		glUniform1i(glGetUniformLocation(splatShader.ID, "u_is_bouncing"), mouse.pressed ? 1 : 0);

		// -----
		glUniform3f(glGetUniformLocation(splatShader.ID, "u_force"), mouseVel.x * 0.1f, mouseVel.y * 0.1f, 0.0f);

		glBindFramebuffer(GL_FRAMEBUFFER, gpuGrid.m_velocityFboB); // write b
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gpuGrid.m_velocityTexA); // read a
		glUniform1i(glGetUniformLocation(splatShader.ID, "u_read_texture"), 0);

		glDrawArrays(GL_TRIANGLES, 0, 6); // run shader
		gpuGrid.swapVelocityBuffers(); // swap a-b

		// add density
		glUniform3f(glGetUniformLocation(splatShader.ID, "u_force"), 5.0f, 0.0f, 0.0f); // +5 dens

		glBindFramebuffer(GL_FRAMEBUFFER, gpuGrid.m_densityFboB); // write b
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gpuGrid.m_densityTexA); // read a
		glUniform1i(glGetUniformLocation(splatShader.ID, "u_read_texture"), 0);

		glDrawArrays(GL_TRIANGLES, 0, 6); // run shader
		gpuGrid.swapDensityBuffers();   // swap a-b

		// diffuseSHADER
		diffuseShader.use();

		// alpha and rbeta
		float a = time_step * viscosity * GRID_WIDTH * GRID_HEIGHT;
		glUniform1f(glGetUniformLocation(diffuseShader.ID, "u_alpha"), a);
		glUniform1f(glGetUniformLocation(diffuseShader.ID, "u_rBeta"), 1.0f / (1.0f + 4.0f * a));

		// diffuse velocity
		// u_b, never changes during the loop
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gpuGrid.m_velocityTexA); // adv res
		glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_b"), 1);

		// iters
		for (int i = 0; i < diffuse_iterations; ++i)
		{
			// set ux to 0
			glActiveTexture(GL_TEXTURE0);
			glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_x"), 0);

			if (i % 2 == 0)
			{
				// read a, write b
				glBindTexture(GL_TEXTURE_2D, gpuGrid.m_velocityTexA);
				glBindFramebuffer(GL_FRAMEBUFFER, gpuGrid.m_velocityFboB);
			}
			else
			{
				// read b, write a
				glBindTexture(GL_TEXTURE_2D, gpuGrid.m_velocityTexB);
				glBindFramebuffer(GL_FRAMEBUFFER, gpuGrid.m_velocityFboA);
			}
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}  // res->m_velocityTexA

		// diffuse density
		//float density_a = time_step * 0.00001f * GRID_WIDTH * GRID_HEIGHT;
		glUniform1f(glGetUniformLocation(diffuseShader.ID, "u_alpha"), a);
		glUniform1f(glGetUniformLocation(diffuseShader.ID, "u_rBeta"), 1.0f / (1.0f + 4.0f * a));

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gpuGrid.m_densityTexA); // adv res
		glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_b"), 1);

		for (int i = 0; i < diffuse_iterations; ++i)
		{
			// set ux to 0
			glActiveTexture(GL_TEXTURE0);
			glUniform1i(glGetUniformLocation(diffuseShader.ID, "u_x"), 0);

			if (i % 2 == 0)
			{
				// read a, write b
				glBindTexture(GL_TEXTURE_2D, gpuGrid.m_densityTexA);
				glBindFramebuffer(GL_FRAMEBUFFER, gpuGrid.m_densityFboB); // Typo fixed: FboB
			}
			else
			{
				// read b, write a
				glBindTexture(GL_TEXTURE_2D, gpuGrid.m_densityTexB);
				glBindFramebuffer(GL_FRAMEBUFFER, gpuGrid.m_densityFboA); // Typo fixed: FboA
			}
			glDrawArrays(GL_TRIANGLES, 0, 6);
		} // res->m_densityTexA

		// PROJECT SHADERS
		// divergenceSHADER
		divergenceShader.use();
		glUniform2f(glGetUniformLocation(divergenceShader.ID, "u_grid_size"), GRID_WIDTH, GRID_HEIGHT);

		glBindFramebuffer(GL_FRAMEBUFFER, gpuGrid.m_divergenceFbo); // write to div
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gpuGrid.m_velocityTexA); // read diff velo
		glUniform1i(glGetUniformLocation(divergenceShader.ID, "u_velocity_field"), 0);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		// pressureSHADER
		pressureShader.use();

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gpuGrid.m_divergenceTex); // b
		glUniform1i(glGetUniformLocation(pressureShader.ID, "u_divergence"), 1);

		for (int i = 0; i < pressure_iterations; ++i)
		{
			glActiveTexture(GL_TEXTURE0);
			glUniform1i(glGetUniformLocation(pressureShader.ID, "u_pressure"), 0);

			if (i % 2 == 0) {
				glBindTexture(GL_TEXTURE_2D, gpuGrid.m_pressureTexA);
				glBindFramebuffer(GL_FRAMEBUFFER, gpuGrid.m_pressureFboB);
			}
			else {
				glBindTexture(GL_TEXTURE_2D, gpuGrid.m_pressureTexB);
				glBindFramebuffer(GL_FRAMEBUFFER, gpuGrid.m_pressureFboA);
			}
			glDrawArrays(GL_TRIANGLES, 0, 6);
		} // res->pressureTexA

		// gradientSHADER
		gradientShader.use();
		glUniform2f(glGetUniformLocation(gradientShader.ID, "u_grid_size"), GRID_WIDTH, GRID_HEIGHT);

		glBindFramebuffer(GL_FRAMEBUFFER, gpuGrid.m_velocityFboB); // write b

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gpuGrid.m_velocityTexA); // read diffed velo
		glUniform1i(glGetUniformLocation(gradientShader.ID, "u_velocity_field"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gpuGrid.m_pressureTexA); // read solved pressure
		glUniform1i(glGetUniformLocation(gradientShader.ID, "u_pressure_field"), 1);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		gpuGrid.swapVelocityBuffers(); // swap, corrected velo(res)->velocityTexA

		// advectSHADER
		advectShader.use();
		glUniform1f(glGetUniformLocation(advectShader.ID, "u_dt"), time_step);
		glUniform2f(glGetUniformLocation(advectShader.ID, "u_grid_size"), GRID_WIDTH, GRID_HEIGHT);

		// advect velocity
		glBindFramebuffer(GL_FRAMEBUFFER, gpuGrid.m_velocityFboB); // write b

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gpuGrid.m_velocityTexA); // vel a as field
		glUniform1i(glGetUniformLocation(advectShader.ID, "u_velocity_field"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gpuGrid.m_velocityTexA); // move vel a
		glUniform1i(glGetUniformLocation(advectShader.ID, "u_quantity_to_move"), 1);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		gpuGrid.swapVelocityBuffers(); // swap a-b
		// RES->A

		// advect density
		glBindFramebuffer(GL_FRAMEBUFFER, gpuGrid.m_densityFboB); // write b

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gpuGrid.m_velocityTexA); // use new vel a
		glUniform1i(glGetUniformLocation(advectShader.ID, "u_velocity_field"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gpuGrid.m_densityTexA); // move dens a
		glUniform1i(glGetUniformLocation(advectShader.ID, "u_quantity_to_move"), 1);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		gpuGrid.swapDensityBuffers(); // swap a-b
		// RES->A

		//// density/velocity input
		//if (mouse.pressed)
		//{
		//	int grid_x = (int)mouse.x;
		//	int grid_y = (int)(GRID_HEIGHT - mouse.y);

		//	//fluidGrid.addDensity(grid_x, grid_y, 50.0f);
		//	//fluidGrid.addVelocity(grid_x, grid_y, .0f, .0f);
		//}

		//fluidGrid.step();

		//// get data from cpu
		//// upload density
		//const auto& density = fluidGrid.getDensity();
		//// bind to upload
		//glBindTexture(GL_TEXTURE_2D, display_texture);
		//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRID_WIDTH, GRID_HEIGHT, GL_RED, GL_FLOAT, density.data());

		//// upload velocity
		//const auto& velocity = fluidGrid.getVelocity();
		//glBindTexture(GL_TEXTURE_2D, velocity_texture);
		//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRID_WIDTH, GRID_HEIGHT, GL_RG, GL_FLOAT, velocity.data());

		//// upload pressure
		//const auto& pressure = fluidGrid.getPressure();
		//glBindTexture(GL_TEXTURE_2D, pressure_texture);
		//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRID_WIDTH, GRID_HEIGHT, GL_RED, GL_FLOAT, pressure.data());

		// RENDERING
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, GRID_WIDTH, GRID_HEIGHT);

		// clear
		glClearColor(.0f, .0f, .0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// draw quad
		quadShader.use();
		glUniform1i(glGetUniformLocation(quadShader.ID, "u_debugMode"), g_DebugMode);

		// bind textures
		//unit0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gpuGrid.getDensityTexture());
		glUniform1i(glGetUniformLocation(quadShader.ID, "u_densityTex"), 0);

		//unit1
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gpuGrid.getVelocityTexture());
		glUniform1i(glGetUniformLocation(quadShader.ID, "u_velocityTex"), 1);

		//unit2
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gpuGrid.getPressureTexture());
		glUniform1i(glGetUniformLocation(quadShader.ID, "u_pressureTex"), 2);


		//// bind textures
		//// unit0
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, display_texture);
		//glUniform1i(glGetUniformLocation(quadShader.ID, "u_densityTex"), 0);

		////unit1
		//glActiveTexture(GL_TEXTURE1);
		//glBindTexture(GL_TEXTURE_2D, velocity_texture);
		//glUniform1i(glGetUniformLocation(quadShader.ID, "u_velocityTex"), 1);

		////unit2
		//glActiveTexture(GL_TEXTURE2);
		//glBindTexture(GL_TEXTURE_2D, pressure_texture);
		//glUniform1i(glGetUniformLocation(quadShader.ID, "u_pressureTex"), 2);

		// draw quad
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// swap buffers
		glfwSwapBuffers(window);
	}

	// clean
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glfwTerminate();
	return 0;
}