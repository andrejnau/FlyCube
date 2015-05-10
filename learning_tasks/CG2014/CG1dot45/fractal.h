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

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STR(s) #s
#define STRV(s) STR(s)

#define POS_ATTRIB 0
#define NORMAL_ATTRIB 1
#define TEXTURE_ATTRIB 2

static const char VERTEX_SHADERF[] =
"#version 300 es\n"
"precision mediump float;\n"
"layout(location = " STRV(POS_ATTRIB) ") in vec2 pos;\n"
"uniform mat4 u_m4MVP;\n"
"void main() {\n"
"    gl_Position = u_m4MVP * vec4(pos, 0.0, 1.0);\n"
"}\n";

static const char FRAGMENT_SHADERF[] =
"#version 300 es\n"
"precision mediump float;\n"
"out vec4 outColor;\n"
"void main() {\n"
"    outColor.rgb = vec3(0.0588, 0.0588, 0.0588);\n"
"    outColor.a = 1.0;\n"
"}\n";

using line = std::pair < glm::vec2, glm::vec2 > ;

struct teragon
{
	int n = 3;
	float eps = 1e-3f;
	int max_it = 5;
	int D = 1;
	int Di = 0;
	int D0 = 0;
};

static float dist(glm::vec2 &a, glm::vec2 &b)
{
	return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

static glm::vec2 rotate(glm::vec2 point, float angle)
{
	glm::vec2 rotated_point;
	rotated_point.x = point.x * cos(angle) - point.y * sin(angle);
	rotated_point.y = point.x * sin(angle) + point.y * cos(angle);
	return rotated_point;
}

class Fractal : public SceneBase
{
public:
	Fractal()
		: axis_x(1.0f, 0.0f, 0.0f),
		axis_y(0.0f, 1.0f, 0.0f),
		axis_z(0.0f, 0.0f, 1.0f)
	{
		koch_template = generate_template();
		polygon = gen_regular_polygon(m_setting.n, 0.8f);
	}

	std::vector<line> generate_template()
	{
		std::vector<line> ret;

		line q;
		q.first = glm::vec2(0.0f, 0.0f);
		q.second = glm::vec2(1.0f, 0.0f);

		float dx = q.second.x - q.first.x;
		float dy = q.second.y - q.first.y;

		dx /= 3;
		dy /= 3;
		line line_tmp;
		line_tmp.first = q.first;
		line_tmp.second = q.first;
		line_tmp.second.x += dx;
		line_tmp.second.y += dy;
		ret.push_back(line_tmp);

		line_tmp.first = line_tmp.second;
		line_tmp.second.x += dx;
		line_tmp.second.y += dy;

		line line_rorate = line_tmp;
		line_rorate.second -= line_rorate.first;
		line_rorate.second = rotate(line_rorate.second, float(acos(-1.0) / 3.0));
		line_rorate.second += line_rorate.first;
		ret.push_back(line_rorate);

		line_rorate = line_tmp;
		line_rorate.first -= line_rorate.second;
		line_rorate.first = rotate(line_rorate.first, float(-acos(-1.0) / 3.0));
		line_rorate.first += line_rorate.second;
		ret.push_back(line_rorate);

		line_tmp.first = line_tmp.second;
		line_tmp.second.x += dx;
		line_tmp.second.y += dy;
		ret.push_back(line_tmp);

		return std::move(ret);
	}

	float get_angle(line q)
	{
		glm::vec2 point = q.second - q.first;
		float alpha = atan2(point.y, point.x);
		return alpha;
	}

	std::vector<line> replace(line q, const std::vector<line> & t, int k, int iter)
	{
		int rD = m_setting.D;
		if (m_setting.Di != 0)
		{
			if (m_setting.Di == 1 && iter % 2 == 0)
				rD = -rD;
			else if (m_setting.Di == -1 && iter % 2 != 0)
				rD = -rD;
		}

		if (m_setting.D0 != 0)
		{
			if (m_setting.D0 == 1 && k % 2 == 0)
				rD = -rD;
			else if (m_setting.D0 == -1 && k % 2 != 0)
				rD = -rD;
		}

		float angle = get_angle(q);
		std::vector<line> ret;

		glm::mat4 ViewTranslate = glm::translate(glm::mat4(1.0f), glm::vec3(q.first, 0.0f));
		glm::mat4 View(1.0f);
		View[0][0] = cos(angle);
		View[1][1] = cos(angle);
		View[0][1] = sin(angle);
		View[1][0] = -sin(angle);
		glm::mat4 Model = glm::scale(glm::mat4(1.0f), glm::vec3(dist(q.first, q.second), dist(q.first, q.second) * rD, 1.0f));

		ViewTranslate = glm::transpose(ViewTranslate);
		View = glm::transpose(View);
		Model = glm::transpose(Model);

		glm::mat4 MVP = Model * View * ViewTranslate;

		for (auto val : t)
		{
			val.first = glm::vec2(glm::vec4(val.first, 0.0f, 1.0f) * MVP);
			val.second = glm::vec2(glm::vec4(val.second, 0.0f, 1.0f) * MVP);
			ret.push_back(val);
		}
		return std::move(ret);
	}

	std::list<line> koch(std::vector<line> q)
	{
		std::list<line> ret(q.begin(), q.end());
		for (int it = 1; it <= m_setting.max_it; ++it)
		{
			float mx = 0;
			for (auto &l : ret)
			{
				mx = std::max(mx, dist(l.first, l.second));
			}
			if (mx < m_setting.eps)
				return std::move(ret);
			ret.clear();
			for (int k = 0; k < (int)q.size(); ++k)
			{
				auto vec = replace(q[k], koch_template, k + 1, it);
				ret.insert(ret.end(), vec.begin(), vec.end());
			}
			q.assign(ret.begin(), ret.end());
		}
		return std::move(ret);
	}

	std::vector<line> gen_regular_polygon(int n, float r)
	{
		std::vector<line> polygon;

		if (n == 1)
		{
			line line_tmp;
			line_tmp.first = glm::vec2(-r, 0.0f);
			line_tmp.second = glm::vec2(r, 0.0f);
			polygon.push_back(line_tmp);
			return std::move(polygon);
		}

		float pi = acos(-1.0f);
		float angle = -(n - 2) * pi / n / 2.0f;
		std::vector<glm::vec2> vertex(n);
		for (int i = 0; i < n; ++i)
		{
			glm::vec2 cur;
			cur.x = r * cos(angle + (2.0f * pi * i) / n);
			cur.y = r * sin(angle + (2.0f * pi * i) / n);
			vertex[i] = cur;
		}
		std::reverse(vertex.begin(), vertex.end());

		for (int i = 0; i < n; ++i)
		{
			line cur;
			cur.first = vertex[i];
			cur.second = vertex[(i + 1) % vertex.size()];
			polygon.push_back(cur);
		}
		return std::move(polygon);
	}

	void gen_teragon()
	{
		vec.clear();

		auto list_line = koch(polygon);
		vec.insert(vec.end(), list_line.begin(), list_line.end());

		init_vao();
	}

	void set_polygon(std::vector<line> && cur_polygon)
	{
		polygon = std::move(cur_polygon);
		if (isInit)
			gen_teragon();
	}

	void set_template(std::vector<line> && cur_polygon)
	{
		koch_template = std::move(cur_polygon);
		if (isInit)
			gen_teragon();
	}

	void set_default_template()
	{
		koch_template = generate_template();
		if (isInit)
			gen_teragon();
	}

	void set_setting(teragon setting)
	{
		if (setting.n != m_setting.n)
			polygon = gen_regular_polygon(setting.n, 0.8f);
		m_setting = setting;
		if (isInit)
			gen_teragon();
	}

	teragon get_setting() const
	{
		return m_setting;
	}

	bool init()
	{
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		mProgram = createProgram(VERTEX_SHADERF, FRAGMENT_SHADERF);
		if (!mProgram)
			return false;

		loc_u_m4MVP = glGetUniformLocation(mProgram, "u_m4MVP");

		glGenVertexArrays(1, &vaoObject);
		glGenBuffers(1, &vboVertex);

		gen_teragon();

		return isInit = true;
	}

	void init_vao()
	{
		glBindVertexArray(vaoObject);
		glBindBuffer(GL_ARRAY_BUFFER, vboVertex);
		glBufferData(GL_ARRAY_BUFFER, 2 * vec.size() * sizeof(glm::vec2), vec.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(POS_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(POS_ATTRIB);
		glBindVertexArray(0);
	}

	void draw()
	{
		glClearColor(0.9411f, 0.9411f, 0.9411f, 1.0f);
		glLineWidth(2);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(mProgram);
		glUniformMatrix4fv(loc_u_m4MVP, 1, GL_FALSE, glm::value_ptr(Matrix));
		glBindVertexArray(vaoObject);
		glDrawArrays(GL_LINES, 0, 2 * vec.size());
		glBindVertexArray(0);
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
	teragon m_setting;

	glm::vec3 axis_x;
	glm::vec3 axis_y;
	glm::vec3 axis_z;

	glm::mat4 Matrix;

	std::vector<line> vec;
	std::vector<line> koch_template;
	std::vector<line> polygon;

	float eps = 1e-2f;

	int m_width;
	int m_height;
	int m_x;
	int m_y;
	bool isInit = false;

	GLuint vboVertex;

	GLuint mProgram;
	GLuint vaoObject;
	GLint loc_u_m4MVP;
};
