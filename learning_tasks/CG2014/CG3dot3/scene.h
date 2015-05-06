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

		/*glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

		glEnable(GL_POLYGON_SMOOTH);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);*/

		return true;
	}

	void read_size()
	{
		std::cout << "enter light: " << std::endl;
		std::cin >> light.x >> light.y;

		std::cout << "enter start_a: " << std::endl;
		std::cin >> start_a.x >> start_a.y;

		std::cout << "enter start_b: " << std::endl;
		std::cin >> start_b.x >> start_b.y;
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
				
		std::vector<glm::vec2> cr;

		float A1, B1, C1, A2, B2, C2;
		std::tie(A1, B1, C1) = getEquation(light, start_a);
		std::tie(A2, B2, C2) = getEquation(start_b, end_b);
		float xcross = -(C1 * B2 - C2 * B1) / (A1 * B2 - A2 * B1);
		float ycross = -(A1 * C2 - A2 * C1) / (A1 * B2 - A2 * B1);
		glm::vec2 cross1(xcross, ycross);
		cr.push_back(cross1);

		std::tie(A1, B1, C1) = getEquation(light, end_a);
		std::tie(A2, B2, C2) = getEquation(start_b, end_b);
		xcross = -(C1 * B2 - C2 * B1) / (A1 * B2 - A2 * B1);
		ycross = -(A1 * C2 - A2 * C1) / (A1 * B2 - A2 * B1);
		glm::vec2 cross2(xcross, ycross);
		cr.push_back(cross2);

		std::tie(A1, B1, C1) = getEquation(light, start_b);
		std::tie(A2, B2, C2) = getEquation(start_a, end_a);
		xcross = -(C1 * B2 - C2 * B1) / (A1 * B2 - A2 * B1);
		ycross = -(A1 * C2 - A2 * C1) / (A1 * B2 - A2 * B1);
		glm::vec2 cross3(xcross, ycross);
		cr.push_back(cross3);

		std::tie(A1, B1, C1) = getEquation(light, end_b);
		std::tie(A2, B2, C2) = getEquation(start_a, end_a);
		xcross = -(C1 * B2 - C2 * B1) / (A1 * B2 - A2 * B1);
		ycross = -(A1 * C2 - A2 * C1) / (A1 * B2 - A2 * B1);
		glm::vec2 cross4(xcross, ycross);
		cr.push_back(cross4);

		modelOfFile.vertices.clear();
		if ((glm::distance(start_b, end_b) + 1e-5  >= glm::distance(start_b, cross1) + glm::distance(cross1, cross2) + glm::distance(cross2, end_b)) |
			(glm::distance(start_b, end_b) + 1e-5 >= glm::distance(start_b, cross2) + glm::distance(cross1, cross2) + glm::distance(cross1, end_b)))
		{
			modelOfFile.vertices.push_back(glm::vec3(cross1, 0.0f));
			modelOfFile.vertices.push_back(glm::vec3(cross2, 0.0f));
		}
		if ((glm::distance(start_a, end_a) + 1e-5 >= glm::distance(start_a, cross3) + glm::distance(cross3, cross4) + glm::distance(cross4, end_a)) ||
			(glm::distance(start_a, end_a) + 1e-5 >= glm::distance(start_a, cross4) + glm::distance(cross3, cross4) + glm::distance(cross3, end_a)))
		{
			modelOfFile.vertices.push_back(glm::vec3(cross3, 0.0f));
			modelOfFile.vertices.push_back(glm::vec3(cross4, 0.0f));
		}
		modelOfFile.init_vao();
		
		glLineWidth(8);
		glUniform4f(shaderLine.uloc_color, 1.0f, 0.0f, 0.0f, 1.0f);
		glBindVertexArray(modelOfFile.vaoObject);
		glDrawArrays(GL_LINES, 0, modelOfFile.vertices.size());
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

	float r = 0.025;
	glm::vec2 light = glm::vec2(0.0, 0.5);
	glm::vec2 start_a = glm::vec2(-0.3, 0.0);
	glm::vec2 end_a = glm::vec2(0.3, 0.0);

	glm::vec2 start_b = glm::vec2(-0.8, -0.3);
	glm::vec2 end_b = glm::vec2(0.8, -0.3);

	ShaderLine shaderLine;
	ModelOfFile modelOfFile;
	ModelOfFile modelOfFile2;

};