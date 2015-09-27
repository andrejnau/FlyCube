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

#include <glm/gtc/matrix_transform.hpp>

#include <simple_camera/camera.h>

class tScenes : public SceneBase, public SingleTon < tScenes >
{
public:
	tScenes()
		: axis_x(1.0f, 0.0f, 0.0f)
		, axis_y(0.0f, 1.0f, 0.0f)
		, axis_z(0.0f, 0.0f, 1.0f)
		, modelOfFile("model/nanosuit/nanosuit.obj")
	{
	}

	virtual bool init()
	{
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.365f, 0.54f, 0.66f, 1.0f);

		lightPosition = glm::vec3(0.0f, 5.0f, 5.0f);
		light_camera.SetCameraPos(lightPosition);

		m_camera.SetCameraPos(glm::vec3(5.0f, 5.0f, 10.0f));

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
		angle += dt;

		light_camera.SetCameraPos(lightPosition);
		light_camera.SetCameraTarget(-lightPosition);

		draw_obj();
	}

	void draw_obj()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shaderLight.program);

		glm::mat4 projection, view, model;
		m_camera.GetMatrix(projection, view, model);

		glUniformMatrix4fv(shaderLight.loc_model, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(shaderLight.loc_view, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(shaderLight.loc_projection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniform3fv(shaderLight.loc_viewPos, 1, glm::value_ptr(m_camera.GetCameraPos()));

		Material material;
		Light light;

		material.ambient = glm::vec3(0.1745, 0.01175, 0.01175);
		material.diffuse = glm::vec3(0.61424, 0.04136, 0.04136);
		material.specular = glm::vec3(0.727811, 0.626959, 0.626959);
		material.shininess = 0.6f;

		glUniform3fv(shaderLight.loc_material.ambient, 1, glm::value_ptr(material.ambient));
		glUniform3fv(shaderLight.loc_material.diffuse, 1, glm::value_ptr(material.diffuse));
		glUniform1f(shaderLight.loc_material.shininess, material.shininess);
		glUniform3fv(shaderLight.loc_material.specular, 1, glm::value_ptr(material.specular));

		light.position = lightPosition;
		light.ambient = glm::vec3(1.0f, 1.0f, 1.0f);
		light.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
		light.specular = glm::vec3(0.25f, 0.0f, 0.0f);

		glUniform3fv(shaderLight.loc_light.position, 1, glm::value_ptr(light.position));
		glUniform3fv(shaderLight.loc_light.ambient, 1, glm::value_ptr(light.ambient));
		glUniform3fv(shaderLight.loc_light.diffuse, 1, glm::value_ptr(light.diffuse));
		glUniform3fv(shaderLight.loc_light.specular, 1, glm::value_ptr(light.specular));

		for (Mesh & cur_mesh : modelOfFile.meshes)
		{
			glBindVertexArray(cur_mesh.VAO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cur_mesh.EBO);
			glBindTexture(GL_TEXTURE_2D, cur_mesh.textures[0].id);
			glDrawElements(GL_TRIANGLES, (GLsizei)cur_mesh.indices.size(), GL_UNSIGNED_INT, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}
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
	struct Material
	{
		glm::vec3 ambient;
		glm::vec3 diffuse;
		glm::vec3 specular;
		float shininess;
	};

	struct Light
	{
		glm::vec3 position;
		glm::vec3 ambient;
		glm::vec3 diffuse;
		glm::vec3 specular;
	};

private:
	float eps = 1e-3f;

	glm::vec3 lightPosition;

	glm::vec3 axis_x;
	glm::vec3 axis_y;
	glm::vec3 axis_z;

	int m_width;
	int m_height;

	float angle = 0.0f;

	Model modelOfFile;

	ShaderLight shaderLight;
	Camera m_camera;
	Camera light_camera;
};