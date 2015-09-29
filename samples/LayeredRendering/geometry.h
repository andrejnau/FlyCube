#pragma once

#include <utilities.h>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct ModelForTexture
{
	ModelForTexture()
	{
		vertices = {
			glm::vec3(-1.0f, 1.0f, 0.0f),
			glm::vec3(1.0f, 1.0f, 0.0f),
			glm::vec3(1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f, -1.0f, 0.0f)
		};

		texcoords = {
			glm::vec2(0.0f, 0.0f),
			glm::vec2(1.0f, 0.0f),
			glm::vec2(1.0f, 1.0f),
			glm::vec2(0.0f, 1.0f)
		};

		indexes = {
			0, 1, 2,
			2, 3, 0
		};

		gen_vao();
		init_vao();
	}

	void init_vao()
	{
		glBindVertexArray(vaoObject);

		glBindBuffer(GL_ARRAY_BUFFER, vboVertex);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(POS_ATTRIB);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ARRAY_BUFFER, vboTexture);
		glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(glm::vec2), texcoords.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(TEXTURE_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(TEXTURE_ATTRIB);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndexe);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size() * sizeof(GLuint), indexes.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void gen_vao()
	{
		glGenBuffers(1, &vboVertex);
		glGenBuffers(1, &vboTexture);
		glGenBuffers(1, &vboIndexe);
		glGenVertexArrays(1, &vaoObject);
	}

	GLuint vboVertex;
	GLuint vboTexture;
	GLuint vboIndexe;
	GLuint vaoObject;

	std::vector<glm::vec3> vertices;
	std::vector<GLuint> indexes;
	std::vector<glm::vec2> texcoords;
};

struct ModelTriangle
{
	ModelTriangle()
	{
		vertices = {
			glm::vec2(-1.0f, -1.0f),
			glm::vec2(-1.0f, 1.0f),
			glm::vec2(1.0f, 1.0f),

			glm::vec2(1.0f, 1.0f),
			glm::vec2(1.0f, -1.0f),
			glm::vec2(-1.0f, -1.0f),
		};

		gen_vao();
		init_vao();
	}

	void init_vao()
	{
		glBindVertexArray(vaoObject);

		glBindBuffer(GL_ARRAY_BUFFER, vboVertex);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec2), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(POS_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(POS_ATTRIB);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(0);
	}

	void gen_vao()
	{
		glGenBuffers(1, &vboVertex);
		glGenVertexArrays(1, &vaoObject);
	}

	GLuint vboVertex;
	GLuint vaoObject;

	std::vector<glm::vec2> vertices;
};