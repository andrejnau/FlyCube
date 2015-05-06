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
		std::string m_path(PROJECT_RESOURCE_MODEL_DIR + file);
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

struct ModelFrustum
{
	ModelFrustum()
	{	
		gen_vao();
	}

	void reset(float r1, float r2, float h, float angle)
	{
		float full_angle = 2.0f * acos(-1.0f);

		glm::vec3 a(r1, h, 0);
		glm::vec3 b(r2, 0, 0);

		float eps = 1e-2f;
		glm::vec3 axis_y(0.0f, 1.0f, 0.0f);

		glm::vec3 prev_a = a;
		glm::vec3 prev_b = b;

		vertices.clear();

		float dx = 0.0f;
		float dy = 0.0f;

		for (float q = 0.0f; q <= full_angle - angle + eps; q += eps)
		{
			glm::mat3 anim = glm::mat3(glm::rotate(glm::mat4(1.0f), -q, axis_y));
			glm::vec3 cur_a = anim * a;
			glm::vec3 cur_b = anim * b;

			vertices.push_back(cur_b);
			vertices.push_back(prev_b);
			vertices.push_back(prev_a);

			vertices.push_back(cur_b);
			vertices.push_back(prev_a);
			vertices.push_back(cur_a);

			dx = std::max(dx, glm::distance(cur_a, prev_a));
			dy = std::max(dy, glm::distance(cur_b, prev_b));

			prev_a = cur_a;
			prev_b = cur_b;
		}

		float L = sqrt(h * h + (r2 - r1) * (r2 - r1));

		prev_a = glm::vec3(r1, h, 0);
		prev_b = glm::vec3(r2, 0, 0);
		
		glm::vec3 axis_x(0.0f, 0.0f, 1.0f);

		for (float q = 0.0f; q <= angle + eps; q += eps)
		{
			glm::vec3 cur_a = prev_a;
			glm::vec3 cur_b = prev_b;

			glm::vec3 v = glm::normalize(cur_a - cur_b);

			glm::mat3 anim = glm::mat3(glm::rotate(glm::mat4(1.0f), -acos(-1.0f) / 2.0f, axis_x));
			v = anim * v;
			
			cur_a += v * dx;
			cur_b += v * dy;		

			vertices.push_back(cur_b);
			vertices.push_back(prev_a);
			vertices.push_back(prev_b);


			vertices.push_back(cur_b);
			vertices.push_back(cur_a);
			vertices.push_back(prev_a);
			

			prev_a = cur_a;
			prev_b = cur_b;
		}
		normals.resize(vertices.size());
		for (int i = 0; i < vertices.size(); )
		{
			glm::vec3 nq = glm::normalize(glm::cross(vertices[i + 1] - vertices[i], vertices[i + 2] - vertices[i]));
			for (int j = 0; j < 3; ++j)
			{
				normals[i++] = nq;
			}
		}

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

struct ModelSuzanne
{
	ModelSuzanne()
	{
		std::string m_path(PROJECT_RESOURCE_MODEL_DIR "suzanne.obj");
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