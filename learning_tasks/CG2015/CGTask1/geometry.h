#pragma once

#include <utilities.h>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct ModelOfFile
{
	ModelOfFile()
	{}

	ModelOfFile(const std::string & file)
	{
		reset(file);
	}

	void reset(const std::string & file)
	{
		std::string m_path(PROJECT_RESOURCE_DIR + file);
		loadOBJ(m_path, vertices, uvs, normals);
		gen_vao();
		init_vao();
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

		glBindBuffer(GL_ARRAY_BUFFER, vboNormal);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(NORMAL_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(NORMAL_ATTRIB);
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

struct ModelOfMemory
{
	ModelOfMemory()
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
		normals.clear();
		normals.resize(vertices.size());
		for (int i = 0; i < (int)vertices.size();)
		{
			glm::vec3 nq = glm::normalize(glm::cross(vertices[i + 1] - vertices[i], vertices[i + 2] - vertices[i]));
			for (int j = 0; j < 3; ++j)
			{
				normals[i++] = nq;
			}
		}

		glBindVertexArray(vaoObject);

		glBindBuffer(GL_ARRAY_BUFFER, vboVertex);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(POS_ATTRIB);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ARRAY_BUFFER, vboNormal);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(NORMAL_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(NORMAL_ATTRIB);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(0);
	}

	GLuint vboVertex;
	GLuint vboNormal;
	GLuint vaoObject;

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
};