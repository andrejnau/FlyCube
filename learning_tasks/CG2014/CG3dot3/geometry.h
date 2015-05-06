#pragma once

#include <utilities.h>
#include <vector>
#include <utils/shaders.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct ModelOfFile
{
	ModelOfFile()
	{
		gen_vao();
	}
	
	void gen_vao()
	{
		glGenBuffers(1, &vboVertex);
		glGenBuffers(1, &vboNormal);
		glGenVertexArrays(1, &vaoObject);
	}

	void init_vao()
	{
		glBindVertexArray(vaoObject);

		glBindBuffer(GL_ARRAY_BUFFER, vboVertex);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(POS_ATTRIB);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(0);
	}

	GLuint vboVertex;
	GLuint vboNormal;
	GLuint vaoObject;

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
};