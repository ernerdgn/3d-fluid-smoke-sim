#pragma once
#include <string>

class Shader
{
public:
	unsigned int ID;

	// read and build shader
	Shader(const char* vertexPath, const char* fragmentPath);
	Shader(const char* computePath);

	// use shader
	void use();
};