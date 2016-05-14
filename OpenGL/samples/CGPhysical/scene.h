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
#include <simple_camera/camera.h>

#include <glm/gtc/matrix_transform.hpp>

class tScenes : public SceneBase
{
public:
	tScenes()
		: axis_x(1.0f, 0.0f, 0.0f)
		, axis_y(0.0f, 1.0f, 0.0f)
		, axis_z(0.0f, 0.0f, 1.0f)
		, modelOfFileBasis("model/newtan_balls/qb.obj")
		, modelOfFileList(balls_count)
	{
		std::string pref = "model/newtan_balls/q";
		std::string suff = ".obj";

		for (int i = 0; i < (int)modelOfFileList.size(); ++i)
		{

			std::string file = pref + std::to_string(i + 1) + suff;
			modelOfFileList[i].reset(file);
			modelOfFileList[i].set_number(i);
		}
	}

	virtual bool init()
	{
		glEnable(GL_DEPTH_TEST);

		glClearColor(0.365f, 0.54f, 0.66f, 1.0f);

		c_textureID = loadCubemap();

		m_camera.SetCameraPos(glm::vec3(0.0f, 0.0f, 2.0f));

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

		glEnable(GL_POLYGON_SMOOTH);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

		modelOfFileList[0].move_ball(-1.8f);

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
		int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		start = std::chrono::system_clock::now();

		for (int i = 0; i < (int)modelOfFileList.size(); ++i)
		{
			for (int j = i + 1; j < (int)modelOfFileList.size(); ++j)
			{
				glm::vec3 a = modelOfFileList[i].get_center();
				glm::vec3 b = modelOfFileList[j].get_center();
				double dist = glm::distance(a, b);
				double mx = 2 * modelOfFileList[i].get_r();

				if (dist - mx < -1e-5)
				{
					std::swap(modelOfFileList[i].speed, modelOfFileList[j].speed);
				}
			}
		}

		for (int i = 0; i < (int)modelOfFileList.size() - 1; ++i)
		{
			glm::vec3 a = modelOfFileList[i].get_center();
			glm::vec3 b = modelOfFileList[i + 1].get_center();
			float dist = glm::distance(a, b);
			float mx = 2 * modelOfFileList[i].get_r();

			if (dist - mx < -1e-5f)
			{
				float eps = 1e-4f;

				modelOfFileList[i].angle_cur += -eps;
				modelOfFileList[i].move_to_angle(modelOfFileList[i].cur_angle);

				modelOfFileList[i + 1].angle_cur += eps;
				modelOfFileList[i + 1].move_to_angle(modelOfFileList[i + 1].cur_angle);
			}
		}

		for (int i = 0; i < (int)modelOfFileList.size(); ++i)
		{
			modelOfFileList[i].update_physical(elapsed / 1e8);
		}

		draw_obj();
		draw_cubemap();
	}

	void draw_obj()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shaderLight.program);

		glm::vec3 lightPosition(cos(angle_light) * sin(angle_light), cos(angle_light), 3.0f);

		glm::mat4 projection, view, model;

		m_camera.GetMatrix(projection, view, model);

		glm::mat4 animX = glm::rotate(glm::mat4(1.0f), 0.0f * float(1.0f * angle / acos(-1.0)), axis_x);
		glm::mat4 animY = glm::rotate(glm::mat4(1.0f), angle, axis_y);

		model = model * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));

		glm::mat4 Matrix = projection * view * model;

		glUniformMatrix4fv(shaderLight.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

		glUniform3fv(shaderLight.loc_lightPosition, 1, glm::value_ptr(lightPosition));
		glUniform3fv(shaderLight.loc_camera, 1, glm::value_ptr(m_camera.GetCameraPos()));

		glUniform1f(shaderLight.loc_isLight, 1.0f);

		for (auto &x : modelOfFileList)
		{
			glBindVertexArray(x.vaoObject);
			glDrawArrays(GL_TRIANGLES, 0, (GLsizei)x.vertices.size());
			glBindVertexArray(0);
		}

		glBindVertexArray(modelOfFileBasis.vaoObject);
		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)modelOfFileBasis.vertices.size());
		glBindVertexArray(0);
	}

	void draw_cubemap()
	{
		glDepthMask(GL_FALSE);
		glUseProgram(shaderSimpleCubeMap.program);
		glBindVertexArray(modelCube.vaoObject);
		glBindTexture(GL_TEXTURE_CUBE_MAP, c_textureID);

		glm::mat4 animX = glm::rotate(glm::mat4(1.0f), float(1.0f * angle / acos(-1.0)), axis_x);
		glm::mat4 animY = glm::rotate(glm::mat4(1.0f), angle, axis_y);
		glm::mat4 animZ = glm::rotate(glm::mat4(1.0f), angle, axis_z);

		glm::mat4 projection, view, model;
		m_camera.GetMatrix(projection, view, model);
		model = model * glm::scale(glm::mat4(1.0f), glm::vec3(4.0f, 4.0f, 4.0f));

		glm::mat4 Matrix = projection * view * model;
		glUniformMatrix4fv(shaderSimpleCubeMap.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)modelCube.vertices.size() / 3);
		glBindVertexArray(0);
		glDepthMask(GL_TRUE);
	}

	std::unique_ptr<unsigned char[]> load_image(const std::string & m_path)
	{
		FILE * p_file = fopen(m_path.c_str(), "rb");
		fseek(p_file, 0, SEEK_END);
		int size = ftell(p_file);
		std::unique_ptr<unsigned char[]> out_file(new unsigned char[size]);

		fread(&out_file, 1, size, p_file);
		return out_file;
	}

	GLuint loadCubemap()
	{
		GLuint textureID;
		glGenTextures(1, &textureID);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

		std::vector <std::string> textures_faces = {
			PROJECT_RESOURCE_DIR "cubemap/skycloud/txStormydays_rt.bmp",
			PROJECT_RESOURCE_DIR "cubemap/skycloud/txStormydays_lf.bmp",
			PROJECT_RESOURCE_DIR "cubemap/skycloud/txStormydays_up.bmp",
			PROJECT_RESOURCE_DIR "cubemap/skycloud/txStormydays_dn.bmp",
			PROJECT_RESOURCE_DIR "cubemap/skycloud/txStormydays_bk.bmp",
			PROJECT_RESOURCE_DIR "cubemap/skycloud/txStormydays_ft.bmp"
		};

		for (GLuint i = 0; i < textures_faces.size(); i++)
		{
			int width, height, channels;
			unsigned char * image = SOIL_load_image(textures_faces[i].c_str(), &width, &height, &channels, SOIL_LOAD_RGB);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, (GLint)GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
			SOIL_free_image_data(image);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, (GLint)GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, (GLint)GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, (GLint)GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, (GLint)GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, (GLint)GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		return textureID;
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

	int m_width;
	int m_height;

	float angle = 0.0;
	float angle_light = 10.0f;

	int balls_count = 6;

	GLuint depthTexture;
	GLuint depthFBO;
	GLuint c_textureID;

	ModelCubeSkybox modelCube;
	ModelPlane modelPlane;
	ShaderShadow shaderShadow;
	ShaderLight shaderLight;
	ShaderShadowView shaderShadowView;
	ShaderSimpleCubeMap shaderSimpleCubeMap;

	std::vector<ModelOfFile> modelOfFileList;
	ModelOfFile modelOfFileBasis;

	Camera m_camera;
};