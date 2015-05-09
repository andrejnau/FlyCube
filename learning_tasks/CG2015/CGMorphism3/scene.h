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

		m_camera.SetPosition(glm::vec3(0, 0, 1));
		m_camera.SetLookAt(glm::vec3(0, 0, 0));
		m_camera.SetClipping(0.1, 100.0);
		m_camera.SetFOV(45);

		m_camera.angle_x = -0.8f;
		m_camera.angle_y = 0.2f;
		m_camera.angle_z = 1.0f;

		for (int i = 0; i < 10; ++i)
		{
			m_camera.Move(CameraDirection::BACK);
			m_camera.Update();
		}

		return true;
	}

	void read_size()
	{
		std::cout << "enter r1: " << std::endl;
		std::cin >> r1;

		std::cout << "enter r2: " << std::endl;
		std::cin >> r2;

		std::cout << "enter h: " << std::endl;
		std::cin >> h;
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

		static bool nn = true;

		if (nn)
			angle += elapsed / 500.0f;
		else
			angle -= elapsed / 500.0f;

		if (angle > 2.0f * acos(-1.0f))
			nn = false;
		if (angle < 0.0f)
			nn = true;

		modelFrustum.reset(r1, r2, h, angle);

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

		glm::mat4 Matrix = projection * view * model;

		glUniformMatrix4fv(shaderLight.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

		glUniform3fv(shaderLight.loc_lightPosition, 1, glm::value_ptr(lightPosition));
		glUniform3fv(shaderLight.loc_camera, 1, glm::value_ptr(camera));				

		glBindVertexArray(modelFrustum.vaoObject);
		glDrawArrays(GL_TRIANGLES, 0, modelFrustum.vertices.size());
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
	glm::vec3 axis_x;
	glm::vec3 axis_y;
	glm::vec3 axis_z;

	float r1 = 0.6f;
	float r2 = 1.0f;
	float h = 1.0f;

	int m_width;
	int m_height;

	float angle = 0.0f;

	ShaderLight shaderLight;
	ModelFrustum modelFrustum;

	Camera m_camera;
};