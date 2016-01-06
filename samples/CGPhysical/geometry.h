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
		s_vertices = vertices;
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

	int number = 0;

	void set_number(int n)
	{
		number = n;

		float x = (float)(0.0f + d * number - 2.5 * d);
		float y = 2.0f;
		float z = 0.0f;

		s_center = center = glm::vec3(x, y, z);
	}

	void move_to_angle(float angle)
	{
		vertices = s_vertices;
		center = s_center;

		float x = (float)(0.0f + d * number - 2.5 * d);
		float y = 0.0f;
		float z = 0.0f;

		for (auto &p : vertices)
		{
			y = std::max(y, p.y);
		}

		for (auto &p : vertices)
		{
			p -= glm::vec3(x, y, z);
		}
		center -= glm::vec3(x, y, z);
		glm::vec3 axis_z(0.0f, 0.0f, 1.0f);
		glm::mat4 anim = glm::rotate(glm::mat4(1.0f), angle, axis_z);

		for (auto &p : vertices)
		{
			glm::vec4 pp(p, 0.0);
			p = glm::vec3(anim * pp);
		}

		glm::vec4 c4 = glm::vec4(center, 0.0);
		center = glm::vec3(anim * c4);
		center += glm::vec3(x, y, z);

		for (auto &p : vertices)
		{
			p += glm::vec3(x, y, z);
		}

		init_vao();

	}

	void move_ball(float angle)
	{
		static bool xx = false;
		if (xx)
			return;
		xx = true;

		angle_cur = angle;
		move_to_angle(angle_cur);
	}

	glm::vec3 get_center()
	{
		return center;
	}

	float get_r()
	{
		return d / 2.0f;
	}

	void update_physical(float dt)
	{
		float df = -g * sin(angle_cur) * dt;
		speed += df;
		angle_cur += speed;

		speed *= 0.998f;

		move_to_angle(angle_cur);
	}

	glm::vec3 center, s_center;

	float d = 0.956f;

	float speed = 0.0;
	float angle_cur = 0.0;

	float cur_angle = 0.0;
	float cur_potential = 0.0;

	float g = 9.81f;
	float angle = 0.0;


	GLuint vboVertex;
	GLuint vboNormal;
	GLuint vaoObject;

	std::vector<glm::vec3> vertices, s_vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
};


struct ModelSuzanne
{
	ModelSuzanne()
	{
		std::string m_path(PROJECT_RESOURCE_DIR "model/suzanne.obj");
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

struct ModelCubeSkybox
{
	ModelCubeSkybox()
	{
		std::string m_path(PROJECT_RESOURCE_DIR "model/cube.obj");

		vertices = {
			-1.0f, 1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, 1.0f, -1.0f,
			-1.0f, 1.0f, -1.0f,

			-1.0f, -1.0f, 1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f, 1.0f, -1.0f,
			-1.0f, 1.0f, -1.0f,
			-1.0f, 1.0f, 1.0f,
			-1.0f, -1.0f, 1.0f,

			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,

			-1.0f, -1.0f, 1.0f,
			-1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 1.0f,
			-1.0f, -1.0f, 1.0f,

			-1.0f, 1.0f, -1.0f,
			1.0f, 1.0f, -1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, -1.0f,

			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f, 1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f, 1.0f,
			1.0f, -1.0f, 1.0f
		};

		gen_vao();
		init_vao();
	}

	void gen_vao()
	{
		glGenBuffers(1, &vboVertex);
		glGenVertexArrays(1, &vaoObject);
	}

	void init_vao()
	{
		glBindVertexArray(vaoObject);

		glBindBuffer(GL_ARRAY_BUFFER, vboVertex);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(POS_ATTRIB);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(0);
	}

	GLuint vboVertex;
	GLuint vaoObject;

	std::vector<GLfloat> vertices;
};


struct ModelPlane
{
	ModelPlane()
	{
		vertices = {
			-10.0, -0.9f, -10.0,
			10.0, -0.9f, -10.0,
			10.0, -0.9f, 10.0,
			-10.0, -0.9f, 10.0
		};

		indexes = {
			0, 1, 2,
			2, 3, 0
		};

		gen_vao();
		init_vao();
	}

	void gen_vao()
	{
		glGenBuffers(1, &vboVertex);
		glGenBuffers(1, &vboIndex);
		glGenVertexArrays(1, &vaoObject);
	}

	void init_vao()
	{
		glBindVertexArray(vaoObject);

		glBindBuffer(GL_ARRAY_BUFFER, vboVertex);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(POS_ATTRIB);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndex);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size() * sizeof(GLuint), indexes.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glBindVertexArray(0);
	}

	GLuint vboVertex;
	GLuint vboIndex;
	GLuint vaoObject;

	std::vector<GLfloat> vertices;
	std::vector<GLuint> indexes;
};