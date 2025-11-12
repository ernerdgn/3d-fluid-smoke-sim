#include <iostream>
#include <glad/glad.h>
#include <glfw3.h>
#include "shader.h"
#include "GpuGrid.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>

float cube_vertices[] = {
	-0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f, -0.5f,
	 0.5f,  0.5f, -0.5f,
	 0.5f,  0.5f, -0.5f,
	-0.5f,  0.5f, -0.5f,
	-0.5f, -0.5f, -0.5f,

	-0.5f, -0.5f,  0.5f,
	 0.5f, -0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	-0.5f,  0.5f,  0.5f,
	-0.5f, -0.5f,  0.5f,

	-0.5f,  0.5f,  0.5f,
	-0.5f,  0.5f, -0.5f,
	-0.5f, -0.5f, -0.5f,
	-0.5f, -0.5f, -0.5f,
	-0.5f, -0.5f,  0.5f,
	-0.5f,  0.5f,  0.5f,

	 0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f, -0.5f,
	 0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,

	-0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f,  0.5f,
	 0.5f, -0.5f,  0.5f,
	-0.5f, -0.5f,  0.5f,
	-0.5f, -0.5f, -0.5f,

	-0.5f,  0.5f, -0.5f,
	 0.5f,  0.5f, -0.5f,
	 0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	-0.5f,  0.5f,  0.5f,
	-0.5f,  0.5f, -0.5f
};

int g_DebugMode = 0; // 0: density, 1: velocity, 2: pressure

GLuint create3DTexture(int width, int height, int depth)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_3D, textureID);

	// set tex params
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// test data
	int size = width * height * depth;
	std::vector<float> data(size);
	for (int z = 0; z < depth; ++z)
	{
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				// wave
				float val = sin(x * 0.1f) * sin(y * 0.1f) + sin(z * 0.1f);
				// normalize
				data[x + y * width + z * width * height] = (val + 1.0f) * .5f;
			}
		}
	}

	// upload data
	glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, width, height, depth, 0, GL_RED, GL_FLOAT, data.data());

	// unbind
	glBindTexture(GL_TEXTURE_3D, 0);
	
	return textureID;
}

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

	const int SCREEN_WIDTH = 512;
	const int SCREEN_HEIGHT = 512;
	glfwSetWindowSize(window, SCREEN_WIDTH, SCREEN_HEIGHT);
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	glEnable(GL_DEPTH_TEST); // depth test
	glEnable(GL_BLEND); // transparency blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // default alpha blending
	glDisable(GL_CULL_FACE); // disable cull

	/* 3D */
	// setup cube vao vbo
	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);

	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

	// read data, 3*float
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// load shaders
	Shader raymarchShader("raymarch.vert", "raymarch.frag");

	// create 3d volume texture
	const int GRID_WIDTH = 128;
	const int GRID_HEIGHT = 128;
	const int GRID_DEPTH = 128;
	GLuint volume_texture = create3DTexture(GRID_WIDTH, GRID_HEIGHT, GRID_DEPTH);

	// cam pos const
	glm::vec3 camera_pos = glm::vec3(0.0, 0.0, 3.0);

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

	glm::vec2 lastMousePos = glm::vec2(.0f);

	// main loop
	while (!glfwWindowShouldClose(window))
	{
		// INPUT
		glfwPollEvents();

		//
		//
		//
		// SIMULATION
		//
		//
		//

		// RENDER
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 3d cam math
		// rayMarhc Shader
		raymarchShader.use();

		// proj mat (field of view)
		glm::mat4 projection = glm::perspective(glm::radians(45.0f),
			(float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);

		// view mat (cam pos)
		// pos 0,0,3 - look at 0,0,0
		glm::mat4 view = glm::lookAt(
			camera_pos,
			glm::vec3(0.0, 0.0, 0.0),
			glm::vec3(0.0, 1.0, 0.0)
		);

		// model mat (obj pos/rot), rotation over time to see 3d
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::rotate(
			model,
			(float)glfwGetTime() * glm::radians(50.0f),
			glm::vec3(.5f, 1.0f, .0f)
		);

		// inv
		glm::mat4 model_inv = glm::inverse(model);

		// send matrices to shader
		glUniformMatrix4fv(glGetUniformLocation(raymarchShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(raymarchShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(raymarchShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

		// send tex and cam pos
		glUniformMatrix4fv(glGetUniformLocation(raymarchShader.ID, "u_model_inv"), 1, GL_FALSE, glm::value_ptr(model_inv));
		glUniform3fv(glGetUniformLocation(raymarchShader.ID, "u_camera_pos"), 1, glm::value_ptr(camera_pos));
		glUniform1i(glGetUniformLocation(raymarchShader.ID, "u_volume_texture"), 0);

		// bind
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, volume_texture);

		// draw cube
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// swap buffers
		glfwSwapBuffers(window);
	}

	// clean
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glfwTerminate();
	return 0;
}