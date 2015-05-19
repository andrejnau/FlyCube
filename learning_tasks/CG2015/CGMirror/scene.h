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
		, modelOfFile("model/character_n.obj")
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

		m_camera.angle_x = 0.5;
		m_camera.angle_y = -0.5;
		m_camera.angle_z = 0.0;

		for (int i = 0; i < 42; ++i)
		{
			m_camera.Move(CameraDirection::BACK);
			m_camera.Update();
		}

		c_textureID = loadCubemap();

		colorTexture = TextureCreate(m_width, m_height);
		depthTexture = TextureCreateDepth(m_width, m_height);
		colorFBO = FBOCreate(colorTexture, depthTexture);

		look_at = glm::vec3(0.0, 0.0, 0.0);
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

		camera = glm::vec3(0.0f, 0.0f, 12.0f);
		glBindTexture(GL_TEXTURE_2D, colorTexture);
		glBindFramebuffer(GL_FRAMEBUFFER, colorFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		draw_obj(true);
		draw_cubemap(true);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		camera = glm::vec3(0.0f, 0.0f, 2.0f);
		draw_obj();
		draw_cubemap();
		draw_fbo();
	}

	void draw_obj(bool is_fbo = false)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shaderLight.program);

		glm::vec3 lightPosition(cos(9.2) * sin(9.2), cos(9.2), 3.0f);

		m_camera.SetLookAt(look_at);
		m_camera.SetPosition(camera);
		m_camera.Update();

		glm::mat4 projection, view, model;

		m_camera.GetMatricies(projection, view, model);

		glm::mat4 Matrix = projection * view * model;

		if (is_fbo)
		{
			glm::mat4 mirror_matrix =
			{
				-1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			};

			Matrix = projection * view * mirror_matrix * model;
		}

		glUniformMatrix4fv(shaderLight.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

		glUniform3fv(shaderLight.loc_lightPosition, 1, glm::value_ptr(lightPosition));
		glUniform3fv(shaderLight.loc_camera, 1, glm::value_ptr(camera));
		glUniform4f(shaderLight.loc_color, 1.0f, 0.0f, 0.0f, 1.0f);

		glBindVertexArray(modelOfFile.vaoObject);
		glDrawArrays(GL_TRIANGLES, 0, modelOfFile.vertices.size());
		glBindVertexArray(0);
	}

	void draw_fbo()
	{
		glUseProgram(shaderTexture.program);

		m_camera.Update();
		glm::mat4 projection, view, model;

		m_camera.GetMatricies(projection, view, model);

		glm::mat4 Matrix = projection * view * model;

		glUniformMatrix4fv(shaderTexture.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

		glBindTexture(GL_TEXTURE_2D, colorTexture);

		glEnableVertexAttribArray(POS_ATTRIB);
		glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, modelPlane.vertices.data());
		glEnableVertexAttribArray(TEXTURE_ATTRIB);
		glVertexAttribPointer(TEXTURE_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, modelPlane.uvs.data());

		glDrawElements(GL_TRIANGLES, modelPlane.indexes.size(), GL_UNSIGNED_INT, modelPlane.indexes.data());

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void draw_cubemap(bool is_fbo = false)
	{
		glDepthMask(GL_FALSE);
		glUseProgram(shaderSimpleCubeMap.program);
		glBindVertexArray(modelCube.vaoObject);
		glBindTexture(GL_TEXTURE_CUBE_MAP, c_textureID);

		m_camera.SetLookAt(look_at);
		m_camera.SetPosition(camera);
		m_camera.Update();

		glm::mat4 projection, view, model;

		m_camera.GetMatricies(projection, view, model);
		model = glm::scale(glm::mat4(1.0f), glm::vec3(40.0f, 40.0f, 40.0f)) * model;

		glm::mat4 Matrix = projection * view * model;

		if (is_fbo)
		{
			glm::mat4 mirror_matrix =
			{
				-1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			};

			Matrix = projection * view * mirror_matrix * model;
		}

		glUniformMatrix4fv(shaderSimpleCubeMap.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

		glDrawArrays(GL_TRIANGLES, 0, modelCube.vertices.size() / 3);
		glBindVertexArray(0);
		glDepthMask(GL_TRUE);
	}

	GLuint FBOCreate(GLuint _Texture, GLuint _Texture_depth)
	{
		GLuint FBO = 0;
		GLenum fboStatus;
		glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _Texture, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _Texture_depth, 0);
		if ((fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE)
		{
			DBG("glCheckFramebufferStatus error 0x%X\n", fboStatus);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return -1;
		}
		// возвращаем FBO по-умолчанию
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return FBO;
	}

	GLuint TextureCreate(GLsizei width, GLsizei height)
	{
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);
		return texture;
	}

	GLuint TextureCreateDepth(GLsizei width, GLsizei height)
	{
		GLuint texture;
		// запросим у OpenGL свободный индекс текстуры
		glGenTextures(1, &texture);
		// сделаем текстуру активной
		glBindTexture(GL_TEXTURE_2D, texture);
		// установим параметры фильтрации текстуры - линейная фильтрация
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// установим параметры "оборачиваниея" текстуры - отсутствие оборачивания
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// необходимо для использования depth-текстуры как shadow map
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		// соаздем "пустую" текстуру под depth-данные
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		return texture;
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
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
			SOIL_free_image_data(image);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
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
	float eps = 1e-3f;

	glm::vec3 axis_x;
	glm::vec3 axis_y;
	glm::vec3 axis_z;

	int m_width;
	int m_height;

	glm::vec3 camera;
	glm::vec3 look_at;

	GLuint colorTexture;
	GLuint colorFBO;
	GLuint c_textureID;

	GLuint depthTexture;

	ShaderLight shaderLight;
	ModelOfFile modelOfFile;
	ShaderSimpleCubeMap shaderSimpleCubeMap;
	ModelCubeSkybox modelCube;

	ModelPlane modelPlane;
	ShaderTexture shaderTexture;

	Camera m_camera;
};