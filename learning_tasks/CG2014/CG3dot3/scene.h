#pragma once
#define GLM_FORCE_RADIANS
#include <scenebase.h>
#include "shaders.h"
#include "geometry.h"
#include <state.h>
#include <ctime>
#include <chrono>
#include <memory>
#include <SOIL.h>
#include <camera.h>

#include <glm/gtc/matrix_transform.hpp>

class tScenes : public SceneBase, public SingleTon < tScenes >
{
public:
	tScenes()
		: axis_x(1.0f, 0.0f, 0.0f)
		, axis_y(0.0f, 1.0f, 0.0f)
		, axis_z(0.0f, 0.0f, 1.0f)
	{
	}

	virtual bool init()
	{
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.365f, 0.54f, 0.66f, 1.0f);

		return true;
	}

	void read_size()
	{
		std::ios_base::sync_with_stdio(false);
		std::cout << "enter light: " << std::endl;
		std::cin >> light.x >> light.y;

		std::cout << "enter start_a: " << std::endl;
		std::cin >> start_a.x >> start_a.y;

		std::cout << "enter end_a: " << std::endl;
		std::cin >> end_a.x >> end_a.y;

		std::cout << "enter start_b: " << std::endl;
		std::cin >> start_b.x >> start_b.y;

		std::cout << "enter end_b: " << std::endl;
		std::cin >> end_b.x >> end_b.y;

		std::cout << std::string(80, '_') << std::endl;
	}

	std::tuple<float, float, float> getEquation(glm::vec2 a, glm::vec2 b)
	{
		glm::vec2 v = b - a;
		float A = -v.y;
		float B = v.x;
		float C = -(A * a.x + B * a.y);
		return std::make_tuple(A, B, C);
	}

	virtual void draw()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		auto & state = CurState<bool>::Instance().state;
		if (state["warframe"])
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now(), end = std::chrono::system_clock::now();

		end = std::chrono::system_clock::now();
		int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		start = std::chrono::system_clock::now();

		modelOfFile.vertices.clear();

		modelOfFile.vertices.push_back(glm::vec3(start_a, 0.0));
		modelOfFile.vertices.push_back(glm::vec3(end_a, 0.0));

		modelOfFile.vertices.push_back(glm::vec3(start_b, 0.0));
		modelOfFile.vertices.push_back(glm::vec3(end_b, 0.0));

		modelOfFile.init_vao();

		float full_angle = 2.0f * acos(-1.0f);
		float eps = 1e-2f;

		glm::vec2 cur(r, 0.0);
		glm::vec2 st = cur;
		glm::vec2 prev = cur;

		modelOfFile2.vertices.clear();

		for (float q = 0.0f; q <= full_angle + eps; q += eps)
		{
			glm::mat2 anim = glm::mat2(glm::rotate(glm::mat4(1.0f), q, axis_z));
			cur = anim * cur;

			modelOfFile2.vertices.push_back(glm::vec3(st + light, 0.0f));
			modelOfFile2.vertices.push_back(glm::vec3(prev + light, 0.0f));
			modelOfFile2.vertices.push_back(glm::vec3(cur + light, 0.0f));

			prev = cur;
		}

		modelOfFile2.init_vao();
		draw_obj();
	}

	void get_ligth(bool is)
	{
		std::vector<glm::vec3> out;
		glm::vec2 cur = start_a;
		float eps = 1e-3f;
		int step = 1000;
		float dx = (end_a.x - start_a.x) / step;
		float dy = (end_a.y - start_a.y) / step;
		float A1, B1, C1, A2, B2, C2;
		float xcross, ycross;

		while (glm::distance(cur, end_a) > eps && is)
		{
			std::tie(A1, B1, C1) = getEquation(light, cur);
			std::tie(A2, B2, C2) = getEquation(start_b, end_b);

			xcross = -(C1 * B2 - C2 * B1) / (A1 * B2 - A2 * B1);
			ycross = -(A1 * C2 - A2 * C1) / (A1 * B2 - A2 * B1);

			glm::vec2 point_for_b = glm::vec2(xcross, ycross);

			if (glm::distance(start_b, point_for_b) + glm::distance(point_for_b, end_b) > glm::distance(start_b, end_b))
			{
				cur += glm::vec2(dx, dy);
				continue;
			}

			if (glm::distance(light, cur) > glm::distance(light, point_for_b))
			{
				out.push_back(glm::vec3(cur, 0.0));
			}

			cur += glm::vec2(dx, dy);
		}

		cur = start_b;
		dx = (end_b.x - start_b.x) / step;
		dy = (end_b.y - start_b.y) / step;

		while (glm::distance(cur, end_b) > eps && !is)
		{
			std::tie(A1, B1, C1) = getEquation(light, cur);
			std::tie(A2, B2, C2) = getEquation(start_a, end_a);

			xcross = -(C1 * B2 - C2 * B1) / (A1 * B2 - A2 * B1);
			ycross = -(A1 * C2 - A2 * C1) / (A1 * B2 - A2 * B1);

			glm::vec2 point_for_a = glm::vec2(xcross, ycross);

			if (glm::distance(start_a, point_for_a) + glm::distance(point_for_a, end_a) > glm::distance(start_a, end_a))
			{
				cur += glm::vec2(dx, dy);
				continue;
			}

			if (glm::distance(light, cur) > glm::distance(light, point_for_a))
			{
				out.push_back(glm::vec3(cur, 0.0));
			}

			cur += glm::vec2(dx, dy);
		}

		modelOfFile.vertices = out;
		modelOfFile.init_vao();
	}

	void draw_obj()
	{
		glClearColor(0.9411f, 0.9411f, 0.9411f, 1.0f);
		glLineWidth(3);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderLine.program);

		glm::mat4 Matrix = glm::mat4(1.0f);

		glUniformMatrix4fv(shaderLine.loc_u_m4MVP, 1, GL_FALSE, glm::value_ptr(Matrix));
		glUniform4f(shaderLine.uloc_color, 0.0f, 0.0f, 0.0f, 1.0f);


		glBindVertexArray(modelOfFile.vaoObject);
		glDrawArrays(GL_LINES, 0, modelOfFile.vertices.size());
		glBindVertexArray(0);

		glLineWidth(0);
		glUniform4f(shaderLine.uloc_color, 1.0f, 1.0f, 0.0f, 1.0f);
		glBindVertexArray(modelOfFile2.vaoObject);
		glDrawArrays(GL_TRIANGLES, 0, modelOfFile2.vertices.size());
		glBindVertexArray(0);

		glLineWidth(6);

		get_ligth(true);
		glUniform4f(shaderLine.uloc_color, 1.0f, 0.0f, 0.0f, 1.0f);
		glBindVertexArray(modelOfFile.vaoObject);
		glDrawArrays(GL_LINE_LOOP, 0, modelOfFile.vertices.size());
		glBindVertexArray(0);

		get_ligth(false);
		glUniform4f(shaderLine.uloc_color, 1.0f, 0.0f, 0.0f, 1.0f);
		glBindVertexArray(modelOfFile.vaoObject);
		glDrawArrays(GL_LINE_LOOP, 0, modelOfFile.vertices.size());
		glBindVertexArray(0);
	}

	virtual void resize(int x, int y, int width, int height)
	{
		glViewport(x, y, width, height);
		m_width = width;
		m_height = height;
	}

	virtual void destroy()
	{
	}

private:
	glm::vec3 axis_x;
	glm::vec3 axis_y;
	glm::vec3 axis_z;

	int m_width;
	int m_height;

	float r = 0.025f;
	glm::vec2 light = glm::vec2(0.5, 0.0);
	glm::vec2 start_a = glm::vec2(0, 0.6);
	glm::vec2 end_a = glm::vec2(0.5, 0.5);

	glm::vec2 start_b = glm::vec2(-0.5, 0.5);
	glm::vec2 end_b = glm::vec2(1, 0.5);

	ShaderLine shaderLine;
	ModelOfFile modelOfFile;
	ModelOfFile modelOfFile2;

};