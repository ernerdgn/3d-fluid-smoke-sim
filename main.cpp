#include <iostream>
#include <glad/glad.h>
#include <glfw3.h>
#include "shader.h"
#include "GpuGrid3D.h"
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

//float quad_vertices[] = {
//	// positions   // texCoords
//	-1.0f,  1.0f,  0.0f, 1.0f,
//	-1.0f, -1.0f,  0.0f, 0.0f,
//	 1.0f, -1.0f,  1.0f, 0.0f,
//	-1.0f,  1.0f,  0.0f, 1.0f,
//	 1.0f, -1.0f,  1.0f, 0.0f,
//	 1.0f,  1.0f,  1.0f, 1.0f
//};

bool rayPlaneIntersect(
	const glm::vec3& rayOrigin,const glm::vec3& rayDir,
	const glm::vec3& planeOrigin, const glm::vec3& planeNormal,
	glm::vec3& outIntersection)
{
	float denom = glm::dot(planeNormal, rayDir);
	if (abs(denom) > 1e-6)  // is parallel
	{
		float t = glm::dot(planeOrigin - rayOrigin, planeNormal) / denom;
		if (t >= 0)
		{
			outIntersection = rayOrigin + t * rayDir;
			return true;
		}
	}
	return false;
}

int g_DebugMode = 0; // 0: density, 1: velocity, 2: pressure
//int g_current_slice = 64;  // start from mid

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
	//glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	glEnable(GL_DEPTH_TEST); // depth test
	glEnable(GL_BLEND); // transparency blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // default alpha blending
	glDisable(GL_CULL_FACE); // disable cull

	/* 3D */
	// setup cube vao vbo
	unsigned int cubeVBO, cubeVAO;
	glGenVertexArrays(1, &cubeVAO);

	glGenBuffers(1, &cubeVBO);

	glBindVertexArray(cubeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

	// read data, 3*float
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// load shaders
	Shader raymarchShader("raymarch.vert", "raymarch.frag");
	Shader clearShader("clear.comp");
	Shader splatShader("splat.comp");
	Shader wireframeShader("wireframe.vert", "wireframe.frag");
	Shader advectShader("advect.comp");
	Shader diffuseShader("diffuse.comp");
	Shader divergenceShader("divergence.comp");
	Shader pressureShader("pressure.comp");
	Shader gradientShader("gradient.comp");

	// create 3d grid
	const int GRID_WIDTH = 64;
	const int GRID_HEIGHT = 64;
	const int GRID_DEPTH = 64;
	GpuGrid3D gpuGrid(GRID_WIDTH, GRID_HEIGHT, GRID_DEPTH);

	// clear grid
	gpuGrid.clear(clearShader);

	
	glm::vec3 camera_pos = glm::vec3(.0, .0, 3.0); // cam pos const
	glm::vec2 total_rotation = glm::vec2(.0f); // mouse rotation
	glm::vec2 last_mouse_pos_cam = glm::vec2(.0f);
	float rotation_speed = .1f;
	float dt = .016f;  // delta time

	float viscosity = .000001f;
	int diffuse_iterations = 4;

	int pressure_iterations = 4;


	////////
	double last_frame_time = glfwGetTime();

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
			//else if (key == GLFW_KEY_W)
			//{
			//	g_current_slice = glm::min(GRID_DEPTH - 1, g_current_slice + 1);
			//	std::cout << "slice: " << g_current_slice << std::endl;
			//}
			//else if (key == GLFW_KEY_S)
			//{
			//	g_current_slice = glm::max(0, g_current_slice - 1);
			//	std::cout << "slice: " << g_current_slice << std::endl;
			//}
		}
	});

	// mouse interaction
	struct MouseState
	{
		bool left_pressed = false;
		bool right_pressed = false;
		double x = 0.0;
		double y = 0.0;
		double lastX_cam = .0;
		double lastY_cam = .0;
	} mouse;

	glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods)
		{
			MouseState* mouse = (MouseState*)glfwGetWindowUserPointer(window);
			if (button == GLFW_MOUSE_BUTTON_LEFT)
			{
				if (action == GLFW_PRESS)
					mouse->left_pressed = true;
				else if (action == GLFW_RELEASE)
					mouse->left_pressed = false;
			}
			else if (button == GLFW_MOUSE_BUTTON_RIGHT)
			{
				if (action == GLFW_PRESS)
				{
					mouse->right_pressed = true;
					mouse->lastX_cam = mouse->x;
					mouse->lastY_cam = mouse->y;
				}
				else if (action == GLFW_RELEASE)
					mouse->right_pressed = false;
			}
		});

	glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos)
		{
			MouseState* mouse = (MouseState*)glfwGetWindowUserPointer(window);
			mouse->x = xpos;
			mouse->y = ypos;
		});

	glfwSetWindowUserPointer(window, &mouse);

	glm::vec3 lastMousePos = glm::vec3(.0f);

	// main loop
	while (!glfwWindowShouldClose(window))
	{
		// time
		double current_time = glfwGetTime();
		dt = (float)(current_time - last_frame_time);
		last_frame_time = current_time;
		//dt = .016f;

		// INPUT
		glfwPollEvents();

		// proj mat (field of view)
		glm::mat4 projection = glm::perspective(glm::radians(45.0f),
			(float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);

		// view mat (cam pos)
		glm::mat4 view = glm::lookAt(
			camera_pos,
			glm::vec3(0.0, 0.0, 0.0),
			glm::vec3(0.0, 1.0, 0.0)
		);

		// model mat (obj pos/rot)
		glm::mat4 model = glm::mat4(1.0f);
		// y-axis rot
		model = glm::rotate(
			model,
			glm::radians(total_rotation.y),
			glm::vec3(.0f, 1.0f, .0f)
		);
		// x-axis rot
		model = glm::rotate(
			model,
			glm::radians(total_rotation.x),
			glm::vec3(1.0f, .0f, .0f)
		);

		// inv
		glm::mat4 model_inv = glm::inverse(model);

		// cam rotation
		if (mouse.right_pressed)
		{
			float deltaX = (float)(mouse.x - mouse.lastX_cam);
			float deltaY = (float)(mouse.y - mouse.lastY_cam);

			total_rotation.y += deltaX * rotation_speed;
			total_rotation.x += deltaY * rotation_speed;

			mouse.lastX_cam = mouse.x;
			mouse.lastY_cam = mouse.y;
		}

		// 3d mouse splat for raycast
		float normX = (2.0f * (float)mouse.x) / SCREEN_WIDTH - 1.0f;
		float normY = 1.0f - (2.0f * (float)mouse.y) / SCREEN_HEIGHT;
		float normZ = 1.0f;

		glm::vec3 ray_ndc = glm::vec3(normX, normY, normZ);

		glm::mat4 invVP = glm::inverse(projection * view);

		glm::vec4 ray_world_4 = invVP * glm::vec4(ray_ndc, 1.0);
		glm::vec3 ray_world = glm::vec3(ray_world_4) / ray_world_4.w;

		glm::vec3 rayOrigin = camera_pos;
		glm::vec3 rayDir = normalize(ray_world - rayOrigin);

		glm::vec3 planeOrigin = glm::vec3(0.0, 0.0, 0.0);
		glm::vec3 planeNormal = glm::vec3(0.0, 0.0, 1.0);
		glm::vec3 intersectionPoint;

		glm::vec3 mousePos3D_grid = glm::vec3(0.0);
		glm::vec3 mouse_vel3D_model = glm::vec3(0.0);
		bool mouseIsIntersecting = false;

		glm::vec3 mousePos3D_world = glm::vec3(0.0);

		if (rayPlaneIntersect(rayOrigin, rayDir, planeOrigin, planeNormal, intersectionPoint))
		{
			mouseIsIntersecting = true;
			mousePos3D_world = intersectionPoint;

			glm::vec3 mouse_vel3D_world = glm::vec3(0.0);
			if (lastMousePos != glm::vec3(0.0f))
			{
				mouse_vel3D_world = (mousePos3D_world - lastMousePos) * 5.0f;
			}
			lastMousePos = mousePos3D_world;

			glm::vec4 model_pos_4 = model_inv * glm::vec4(mousePos3D_world, 1.0);
			glm::vec3 mousePos3D_model = glm::vec3(model_pos_4) / model_pos_4.w;

			glm::vec4 model_vel_4 = model_inv * glm::vec4(mouse_vel3D_world, 0.0);
			mouse_vel3D_model = glm::vec3(model_vel_4);

			mousePos3D_grid = (mousePos3D_model + 0.5f) * glm::vec3(GRID_WIDTH, GRID_HEIGHT, GRID_DEPTH);
		}
		else
		{
			lastMousePos = glm::vec3(0.0f);
		}

		/////////////////////////////////////////////////////
		// step
		gpuGrid.step(splatShader, advectShader, diffuseShader,
			divergenceShader, pressureShader, gradientShader,
			mousePos3D_grid, mouse_vel3D_model, // <-- Pass the new corrected values
			mouse.left_pressed && mouseIsIntersecting,
			dt,
			viscosity, diffuse_iterations, pressure_iterations);
		/////////////////////////////////////////////////////

		// render
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// pass raymarch
		raymarchShader.use();

		glUniformMatrix4fv(glGetUniformLocation(raymarchShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(raymarchShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(raymarchShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(glGetUniformLocation(raymarchShader.ID, "u_model_inv"), 1, GL_FALSE, glm::value_ptr(model_inv));
		glUniform3fv(glGetUniformLocation(raymarchShader.ID, "u_camera_pos"), 1, glm::value_ptr(camera_pos));
		glUniform1i(glGetUniformLocation(raymarchShader.ID, "u_volume_texture"), 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, gpuGrid.getDensityTexture());

		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// --- Pass 2: Wireframe ---
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		wireframeShader.use();

		// --- NO REDEFINITION ---
		glUniformMatrix4fv(glGetUniformLocation(wireframeShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(wireframeShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(wireframeShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glfwSwapBuffers(window);
	}

	// clean
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &cubeVBO);
	glfwTerminate();
	return 0;
}