#pragma once
#include <platform.h>
#include <utilities.h>
#include <scenebase.h>
#include <algorithm>
#include <math.h>
#include <string>
#include <vector>
#include <list>
#include <ctime>
#include <chrono>

#include <util.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STR(s) #s
#define STRV(s) STR(s)

#define POS_ATTRIB 0
#define NORMAL_ATTRIB 1
#define TEXTURE_ATTRIB 2

static const char VERTEX_SHADERP[] =
"#version 300 es\n"
"precision mediump float;\n"
"layout(location = " STRV(POS_ATTRIB) ") in vec2 pos;\n"
"uniform mat4 u_m4MVP;\n"
"uniform vec4 u_color;\n"
"out vec4 _color;\n"
"void main() {\n"
"    _color = u_color;\n"
"    gl_Position = u_m4MVP * vec4(pos, 0.0, 1.0);\n"
"}\n";

static const char FRAGMENT_SHADERP[] =
"#version 300 es\n"
"precision mediump float;\n"
"out vec4 outColor;\n"
"in vec4 _color;\n"
"void main() {\n"
"    outColor = _color;\n"
"}\n";

class PrintPolygon : public SceneBase, public SingleTon < PrintPolygon >
{
public:
	PrintPolygon()
	{
	}

	void set_based(const std::vector<glm::vec2> &vec, const glm::vec4 &color)
	{
		m_based_vertex = vec;
		m_based_color = color;
		f_input_line = false;
		point.clear();
		init_vao_based();
		init_vao();
	}

	void set_Beziers(const std::vector<glm::vec2> &vec, const glm::vec4 &color)
	{
		m_Beziers_vertex = vec;
		m_Beziers_color = color;
		f_input_line = false;
		point.clear();
		init_vao_Beziers();
		init_vao();
	}

	bool add_point(glm::vec2 val)
	{
		bool ret = false;
		f_input_line = false;

		float eps = 30.0f / std::max(m_width, m_height);
		if (point.size() > 1 && dist(point.front(), val) < eps)
		{
			val = point.front();
			ret = true;
		}

		point.push_back(val);
		if (point.size() > 1)
		{
			vec.push_back(line(point[point.size() - 2], val));
		}
		init_vao();
		return ret;
	}

	void future_point(glm::vec2 val)
	{
		if (point.size() >= 1)
		{
			input_line = line(point[point.size() - 1], val);
			f_input_line = true;
		}
		init_vao();
	}

	void clear()
	{
		point.clear();
		vec.clear();
		f_input_line = false;
		init_vao();
		m_based_vertex.clear();
		m_Beziers_vertex.clear();
		init_vao_based();
	}

	bool init()
	{
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		mProgram = createProgram(VERTEX_SHADERP, FRAGMENT_SHADERP);
		if (!mProgram)
			return false;

		loc_u_m4MVP = glGetUniformLocation(mProgram, "u_m4MVP");

		glGenVertexArrays(1, &vaoObjectPoint);
		glGenBuffers(1, &vboVertexPoint);

		glGenVertexArrays(1, &vaoObjectLine);
		glGenBuffers(1, &vboVertexLine);

		glGenVertexArrays(1, &vaoObjectBased);
		glGenBuffers(1, &vboVertexBased);

		glGenVertexArrays(1, &vaoObjectBeziers);
		glGenBuffers(1, &vboVertexBeziers);

		glGenVertexArrays(1, &vaoObjectcurLine);
		glGenBuffers(1, &vboVertexcurLine);

		uloc_color = glGetUniformLocation(mProgram, "u_color");

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

		glEnable(GL_POLYGON_SMOOTH);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

		return isInit = true;
	}

	void init_vao()
	{
		glBindVertexArray(vaoObjectPoint);
		glBindBuffer(GL_ARRAY_BUFFER, vboVertexPoint);
		glBufferData(GL_ARRAY_BUFFER, 2 * point.size() * sizeof(glm::vec2), point.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(POS_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(POS_ATTRIB);
		glBindVertexArray(0);

		glBindVertexArray(vaoObjectLine);
		glBindBuffer(GL_ARRAY_BUFFER, vboVertexLine);
		glBufferData(GL_ARRAY_BUFFER, 2 * vec.size() * sizeof(glm::vec2), vec.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(POS_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(POS_ATTRIB);
		glBindVertexArray(0);

		glBindVertexArray(vaoObjectcurLine);
		glBindBuffer(GL_ARRAY_BUFFER, vboVertexcurLine);
		glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(glm::vec2), &input_line, GL_STATIC_DRAW);
		glVertexAttribPointer(POS_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(POS_ATTRIB);
		glBindVertexArray(0);
	}

	void init_vao_based()
	{
		glBindVertexArray(vaoObjectBased);
		glBindBuffer(GL_ARRAY_BUFFER, vboVertexBased);
		glBufferData(GL_ARRAY_BUFFER, 2 * m_based_vertex.size() * sizeof(glm::vec2), m_based_vertex.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(POS_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(POS_ATTRIB);
		glBindVertexArray(0);
	}

	void init_vao_Beziers()
	{
		glBindVertexArray(vaoObjectBeziers);
		glBindBuffer(GL_ARRAY_BUFFER, vboVertexBeziers);
		glBufferData(GL_ARRAY_BUFFER, 2 * m_Beziers_vertex.size() * sizeof(glm::vec2), m_Beziers_vertex.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(POS_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(POS_ATTRIB);
		glBindVertexArray(0);
	}

	void draw_based()
	{
		glUseProgram(mProgram);
		glUniformMatrix4fv(loc_u_m4MVP, 1, GL_FALSE, glm::value_ptr(Matrix));
		glUniform4f(uloc_color, m_based_color.r, m_based_color.g, m_based_color.b, m_based_color.a);

		glBindVertexArray(vaoObjectBased);
		glDrawArrays(GL_LINE_STRIP, 0, m_based_vertex.size());
		glBindVertexArray(0);
	}

	void draw_Beziers()
	{
		glUseProgram(mProgram);
		glUniformMatrix4fv(loc_u_m4MVP, 1, GL_FALSE, glm::value_ptr(Matrix));
		glUniform4f(uloc_color, m_Beziers_color.r, m_Beziers_color.g, m_Beziers_color.b, m_Beziers_color.a);

		glBindVertexArray(vaoObjectBeziers);
		glDrawArrays(GL_LINE_STRIP, 0, m_Beziers_vertex.size());
		glBindVertexArray(0);
	}

	void draw()
	{
		glClearColor(0.9411f, 0.9411f, 0.9411f, 1.0f);
		glLineWidth(2);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(mProgram);
		glUniformMatrix4fv(loc_u_m4MVP, 1, GL_FALSE, glm::value_ptr(Matrix));
		glUniform4f(uloc_color, 0.0588f, 0.0588f, 0.0588f, 1.0f);

		glBindVertexArray(vaoObjectLine);
		glDrawArrays(GL_LINES, 0, 2 * vec.size());
		glBindVertexArray(0);

		if (f_input_line)
		{
			glBindVertexArray(vaoObjectcurLine);
			glDrawArrays(GL_LINES, 0, 2);
			glBindVertexArray(0);
		}

		glBindVertexArray(vaoObjectPoint);
		glDrawArrays(GL_POINTS, 0, point.size());
		glBindVertexArray(0);

		draw_based();
		draw_Beziers();
	}

	void resize(int x, int y, int width, int height)
	{
		m_x = x;
		m_y = y;
		m_width = width;
		m_height = height;

		glViewport(m_x, m_y, m_width, m_height);
		GLfloat ratio = (GLfloat)m_height / (GLfloat)m_width;
		if (m_width >= m_height)
			Matrix = glm::ortho(-1.0f / ratio, 1.0f / ratio, -1.0f, 1.0f, -10.0f, 1.0f);
		else
			Matrix = glm::ortho(-1.0f, 1.0f, -1.0f*ratio, 1.0f*ratio, -10.0f, 1.0f);
	}
	void destroy()
	{
	}
private:
	line input_line;
	bool f_input_line;
	bool isInput;
	glm::mat4 Matrix;

	std::vector<glm::vec2> point;
	std::vector<line> vec;

	int m_width;
	int m_height;
	int m_x;
	int m_y;
	bool isInit = false;

	GLuint mProgram;

	GLuint vaoObjectcurLine;
	GLuint vboVertexcurLine;

	GLuint vaoObjectPoint;
	GLuint vboVertexPoint;

	GLuint vaoObjectLine;
	GLuint vboVertexLine;

	GLuint vaoObjectBased;
	GLuint vboVertexBased;

	GLuint vaoObjectBeziers;
	GLuint vboVertexBeziers;

	GLint loc_u_m4MVP;
	GLint uloc_color;
	std::vector < glm::vec2> m_based_vertex;
	glm::vec4 m_based_color;

	std::vector <glm::vec2> m_Beziers_vertex;
	glm::vec4 m_Beziers_color;
};