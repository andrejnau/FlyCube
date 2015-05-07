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

#define ONE_LOOP

#include <glm/gtc/matrix_transform.hpp>

class tScenes : public SceneBase, public SingleTon < tScenes >
{
public:
	tScenes()
		: axis_x(1.0f, 0.0f, 0.0f)
		, axis_y(0.0f, 1.0f, 0.0f)
		, axis_z(0.0f, 0.0f, 1.0f)
		, modelOfMemoryL(4)
	{
	}

	virtual bool init()
	{
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.365f, 0.54f, 0.66f, 1.0f);

		m_camera.SetMode(FREE);
		m_camera.SetPosition(glm::vec3(0, 0, 1));
		m_camera.SetLookAt(glm::vec3(0, 0, 0));
		m_camera.SetClipping(0.1, 100.0);
		m_camera.SetFOV(45);

		m_camera.angle_x = 0.5;
		m_camera.angle_y = -0.5;
		m_camera.angle_z = 0.0;

		for (int i = 0; i < 42; ++i)
		{
			m_camera.Move(BACK);
			m_camera.Update();
		}

		glm::vec3 a(-1.0f, 1.0f + eps, 1.0f);
		glm::vec3 b(1.0f, 1.0f + eps, 1.0f);
		glm::vec3 c(1.0f, 1.0f + eps, -1.0f);
		glm::vec3 d(-1.0f, 1.0f + eps, -1.0f);

		modelOfMemoryL[0].vertices.push_back(d);
		modelOfMemoryL[0].vertices.push_back(a);
		modelOfMemoryL[0].vertices.push_back(b);

		modelOfMemoryL[0].vertices.push_back(b);
		modelOfMemoryL[0].vertices.push_back(c);
		modelOfMemoryL[0].vertices.push_back(d);

		lenta_a = b;
		lenta_b = c;

		for (int i = 1; i < modelOfMemoryL.size(); ++i)
		{
			auto vec = modelOfMemoryL[i - 1].vertices;
			for (int j = 0; j < vec.size(); ++j)
			{
				glm::mat3 r = glm::mat3(glm::rotate(glm::mat4(1.0f), acos(-1.0f) / 2.0f, this->axis_z));
				vec[j] = r * vec[j];
			}
			modelOfMemoryL[i].vertices = vec;
		}

		for (int i = 0; i < modelOfMemoryL.size(); ++i)
		{
			modelOfMemoryL[i].init_vao();
		}

		//modelOfMemoryL[0].p

		modelOfFile.reset("parallelepiped.obj");
		return true;
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

		float dt = std::min(0.001f, elapsed / 1500.0f);

#ifdef ONE_LOOP
		if (!full_turn)
#endif
			angle += dt;

		glm::vec3 g(-1.0f, 1.0f + eps, -1.0f);
		glm::mat3 rotate = glm::mat3(glm::rotate(glm::mat4(1.0f), angle, this->axis_z));

#ifdef ONE_LOOP
		for (int i = 0; i < 4; ++i)
#else
		for (int i = 0; i < 1000; ++i)
#endif
		{
			float m_angle = i * acos(-1.0) / 2.0f;
			if (fabs(m_angle - angle) < eps)
			{
				cur_id = i % 4;
				break;
			}

		}

		if (angle > 2.0 * acos(-1.0f))
		{
			full_turn = true;
		}

#ifdef ONE_LOOP
		if (!full_turn)
#endif
		{
			lenta_a.x += dt;
			lenta_b.x += dt;
		}

		glm::vec3 lenta_c = glm::inverse(rotate) * modelOfMemoryL[cur_id].vertices[1];
		glm::vec3 lenta_d = glm::inverse(rotate) * modelOfMemoryL[cur_id].vertices[0];

		modelLenta.vertices.clear();
		modelLenta.vertices.push_back(lenta_d);
		modelLenta.vertices.push_back(lenta_c);
		modelLenta.vertices.push_back(lenta_a);

		modelLenta.vertices.push_back(lenta_a);
		modelLenta.vertices.push_back(lenta_b);
		modelLenta.vertices.push_back(lenta_d);
		modelLenta.init_vao();

		draw_obj();
	}

	void draw_obj()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shaderLight.program);

		glm::vec3 lightPosition(cos(9.2) * sin(9.2), cos(9.2), 3.0f);
		glm::vec3 camera(0.0f, 0.0f, 2.0f);

		m_camera.SetLookAt(glm::vec3(0, 0, 0));
		m_camera.SetPosition(camera);
		m_camera.Update();

		glm::mat4 projection, view, model;

		m_camera.GetMatricies(projection, view, model);

		model = model * glm::rotate(glm::mat4(1.0f), -angle, this->axis_z);

		glm::mat4 Matrix = projection * view * model;

		glUniformMatrix4fv(shaderLight.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

		glUniform3fv(shaderLight.loc_lightPosition, 1, glm::value_ptr(lightPosition));
		glUniform3fv(shaderLight.loc_camera, 1, glm::value_ptr(camera));
		glUniform4f(shaderLight.loc_color, 0.5f, 0.5f, 0.5f, 1.0f);

		glBindVertexArray(modelOfFile.vaoObject);
		glDrawArrays(GL_TRIANGLES, 0, modelOfFile.vertices.size());
		glBindVertexArray(0);

		glUniform4f(shaderLight.loc_color, 1.0f, 0.0f, 0.0f, 1.0f);

#ifdef ONE_LOOP
		for (int i = cur_id + 1; i < modelOfMemoryL.size(); ++i)
#else
		for (int i = 0; i < modelOfMemoryL.size(); ++i)
#endif
		{
			glBindVertexArray(modelOfMemoryL[i].vaoObject);
			glDrawArrays(GL_TRIANGLES, 0, modelOfMemoryL[i].vertices.size());
			glBindVertexArray(0);
		}
		m_camera.GetMatricies(projection, view, model);
		Matrix = projection * view * model;
		glUniformMatrix4fv(shaderLight.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

		glBindVertexArray(modelLenta.vaoObject);
		glDrawArrays(GL_TRIANGLES, 0, modelLenta.vertices.size());
		glBindVertexArray(0);
	}

	virtual void resize(int x, int y, int width, int height)
	{
		glViewport(x, y, width, height);
		m_width = width;
		m_height = height;
		m_camera.SetViewport(x, y, width, height);
	}

	virtual void destroy()
	{
	}

	Camera & getCamera()
	{
		return m_camera;
	}

private:
	float eps = 1e-3f;

	glm::vec3 axis_x;
	glm::vec3 axis_y;
	glm::vec3 axis_z;

	glm::vec3 lenta_a;
	glm::vec3 lenta_b;

	float r1 = 0.6;
	float r2 = 1;
	float h = 1;

	int m_width;
	int m_height;

	float angle = 0.0f;

	int cur_id = 0;

	bool full_turn = false;

	ShaderLight shaderLight;
	ModelOfFile modelOfFile;
	std::vector<ModelOfMemory> modelOfMemoryL;
	ModelOfMemory modelLenta;

	Camera m_camera;
};