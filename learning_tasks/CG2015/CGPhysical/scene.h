#pragma once

#include <scenebase.h>
#include "shaders.h"
#include "geometry.h"
#include <state.h>
#include <ctime>
#include <chrono>

#include <glm/gtc/matrix_transform.hpp>

class tScenes : public SceneBase, public SingleTon < tScenes >
{
public:
	tScenes()
		: axis_x(1.0f, 0.0f, 0.0f)
		, axis_y(0.0f, 1.0f, 0.0f)
		, axis_z(0.0f, 0.0f, 1.0f)
	{}

	virtual bool init()
	{
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.365f, 0.54f, 0.66f, 1.0f);

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

		angle_light += elapsed / 1000.0f;
		//angle += elapsed / 250.0f;

		draw_in_depth();
		draw_obj();
	}

	void draw_obj()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shaderLight.program);

		glm::vec3 lightPosition(sin(angle_light), cos(angle_light), 1.0f);
		glm::vec3 camera(0.0f, 0.0f, 2.0f);

		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 view = glm::lookAt(
			camera,
			glm::vec3(0, 0, 0),
			glm::vec3(0, 1, 0)
			);
		glm::mat4 projection = glm::perspective(
			45.0f,
			(float)m_width / m_height,
			0.1f,
			100.0f
			);

		glm::mat4 animX = glm::rotate(glm::mat4(1.0f), 0.0f * float(1.0f * angle / acos(-1.0)), axis_x);
		glm::mat4 animY = glm::rotate(glm::mat4(1.0f), angle, axis_y);

		model = animX * animY * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));

		glm::mat4 Matrix = projection * view * model;

		glUniformMatrix4fv(shaderLight.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

		glUniform3fv(shaderLight.loc_lightPosition, 1, glm::value_ptr(lightPosition));
		glUniform3fv(shaderLight.loc_camera, 1, glm::value_ptr(camera));

		{
			glm::mat4 biasMatrix(
				0.5, 0.0, 0.0, 0.0,
				0.0, 0.5, 0.0, 0.0,
				0.0, 0.0, 0.5, 0.0,
				0.5, 0.5, 0.5, 1.0
				);

			glm::mat4 view = glm::lookAt(
				lightPosition,
				glm::vec3(0, 0, 0),
				glm::vec3(0, 1, 0)
				);

			glm::mat4 Matrix = projection * view * model;

			glm::mat4 depthBiasMVP = biasMatrix * Matrix;

			glUniformMatrix4fv(shaderLight.loc_DepthBiasMVP, 1, GL_FALSE, glm::value_ptr(depthBiasMVP));
		}

		glUniform1i(shaderLight.loc_isLight, 1);

		glBindVertexArray(modelSuzanne.vaoObject);
		glDrawArrays(GL_TRIANGLES, 0, modelSuzanne.vertices.size());
		glBindVertexArray(0);

		glUniform1i(shaderLight.loc_isLight, 0);

		glBindVertexArray(modelPlane.vaoObject);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelPlane.vboIndex);
		glDrawElements(GL_TRIANGLES, modelPlane.indexes.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	void draw_obj_depth()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shaderShadow.program);

		glm::vec3 lightPosition(sin(angle_light), cos(angle_light), 1.0f);

		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 view = glm::lookAt(
			lightPosition,
			glm::vec3(0, 0, 0),
			glm::vec3(0, 1, 0)
			);
		glm::mat4 projection = glm::perspective(
			45.0f,
			(float)m_width / m_height,
			0.1f,
			100.0f
			);

		glm::mat4 animX = glm::rotate(glm::mat4(1.0f), 0.0f * float(1.0f * angle / acos(-1.0)), axis_x);
		glm::mat4 animY = glm::rotate(glm::mat4(1.0f), angle, axis_y);

		model = animX * animY * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));

		glm::mat4 Matrix = projection * view * model;

		glUniformMatrix4fv(shaderShadow.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

		glBindVertexArray(modelSuzanne.vaoObject);
		glDrawArrays(GL_TRIANGLES, 0, modelSuzanne.vertices.size());
		glBindVertexArray(0);
	}

	void draw_in_depth()
	{
		// установим активный FBO
		glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
		// включаем вывод буфера глубины
		glDepthMask(GL_TRUE);
		// очищаем буфер глубины перед его заполнением
		glClear(GL_DEPTH_BUFFER_BIT);
		// выполним рендер сцены с использованием шейдерной программы и камеры источника освещения
		draw_obj_depth();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	GLuint FBOCreateDepth(GLuint _depthTexture)
	{
		// Framebuffer Object (FBO) для рендера в него буфера глубины
		GLuint depthFBO = 0;
		// переменная для получения состояния FBO
		GLenum fboStatus;
		// создаем FBO для рендера глубины в текстуру
		glGenFramebuffers(1, &depthFBO);
		// делаем созданный FBO текущим
		glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
		// отключаем вывод цвета в текущий FBO
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		// указываем для текущего FBO текстуру, куда следует производить рендер глубины
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture, 0);
		// проверим текущий FBO на корректность
		if ((fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE)
		{
			DBG("glCheckFramebufferStatus error 0x%X\n", fboStatus);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return -1;
		}
		// возвращаем FBO по-умолчанию
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return depthFBO;
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		// соаздем "пустую" текстуру под depth-данные
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
		return texture;
	}

	virtual void resize(int x, int y, int width, int height)
	{
		glViewport(x, y, width, height);
		m_width = width;
		m_height = height;

		depthTexture = TextureCreateDepth(m_width, m_height);
		depthFBO = FBOCreateDepth(depthTexture);
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

	float angle = 0.0;
	float angle_light = 0.0f;

	GLuint depthTexture;
	GLuint depthFBO;

	ModelSuzanne modelSuzanne;
	ModelPlane modelPlane;
	ShaderShadow shaderShadow;
	ShaderLight shaderLight;
};