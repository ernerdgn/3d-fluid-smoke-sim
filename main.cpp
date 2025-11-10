#include <iostream>
#include <glad/glad.h>
#include <glfw3.h>
#include "shader.h"
#include "FluidGrid.h"
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
const int GRID_WIDTH = 128;
const int GRID_HEIGHT = 128;

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

	Shader quadShader("quad.vert", "quad.frag");

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

	FluidGrid fluidGrid(GRID_WIDTH, GRID_HEIGHT);

	// texture
	unsigned int display_texture;
	glGenTextures(1, &display_texture);
	glBindTexture(GL_TEXTURE_2D, display_texture);

	// tex params
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// allocate texture on gpu
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, GRID_WIDTH, GRID_HEIGHT, 0, GL_RED, GL_FLOAT, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0); // unbind

	// velocity and pressure textures (for debug display)
	// velocity
	unsigned int velocity_texture;
	glGenTextures(1, &velocity_texture);
	glBindTexture(GL_TEXTURE_2D, velocity_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// red+green for velocity
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, GRID_WIDTH, GRID_HEIGHT, 0, GL_RG, GL_FLOAT, NULL);

	// pressure
	unsigned int pressure_texture;
	glGenTextures(1, &pressure_texture);
	glBindTexture(GL_TEXTURE_2D, pressure_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// red for pressure
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, GRID_WIDTH, GRID_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0); // unbind

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

	// main loop
	while (!glfwWindowShouldClose(window))
	{
		// poll events
		glfwPollEvents();

		// density/velocity input
		if (mouse.pressed)
		{
			int grid_x = (int)mouse.x;
			int grid_y = (int)(GRID_HEIGHT - mouse.y);

			fluidGrid.addDensity(grid_x, grid_y, 50.0f);
			//fluidGrid.addVelocity(grid_x, grid_y, .0f, .0f);
		}

		fluidGrid.step();

		// get data from cpu
		// upload density
		const auto& density = fluidGrid.getDensity();
		// bind to upload
		glBindTexture(GL_TEXTURE_2D, display_texture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRID_WIDTH, GRID_HEIGHT, GL_RED, GL_FLOAT, density.data());

		// upload velocity
		const auto& velocity = fluidGrid.getVelocity();
		glBindTexture(GL_TEXTURE_2D, velocity_texture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRID_WIDTH, GRID_HEIGHT, GL_RG, GL_FLOAT, velocity.data());

		// upload pressure
		const auto& pressure = fluidGrid.getPressure();
		glBindTexture(GL_TEXTURE_2D, pressure_texture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRID_WIDTH, GRID_HEIGHT, GL_RED, GL_FLOAT, pressure.data());

		// RENDERING
		// clear
		glClearColor(.0f, .0f, .0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// draw quad
		quadShader.use();
		glUniform1i(glGetUniformLocation(quadShader.ID, "u_debugMode"), g_DebugMode);

		// bind textures
		// unit0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, display_texture);
		glUniform1i(glGetUniformLocation(quadShader.ID, "u_densityTex"), 0);

		//unit1
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, velocity_texture);
		glUniform1i(glGetUniformLocation(quadShader.ID, "u_velocityTex"), 1);

		//unit2
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, pressure_texture);
		glUniform1i(glGetUniformLocation(quadShader.ID, "u_pressureTex"), 2);

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