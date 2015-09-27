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
		, modelOfFile("model/character_n.obj")
		, modelOfFileSphere("model/sphere.obj")
		, modelOfFileAnnie("model/annie.obj")
	{
	}

	virtual bool init()
	{
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.365f, 0.54f, 0.66f, 1.0f);

		c_textureID = loadCubemap();

		depthTexture = TextureCreateDepth(m_width, m_height);
		depthFBO = FBOCreate(depthTexture);

		lightPosition = glm::vec3(0.0f, 5.0f, 5.0f);
		light_camera.SetCameraPos(lightPosition);

		camera = glm::vec3(5.0f, 5.0f, 10.0f);
		m_camera.SetCameraPos(camera);

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

		lightPosition = glm::vec3(10.0f * cos(angle), 10.0f, 10.0f * sin(angle));
		light_camera.SetCameraPos(lightPosition);
		light_camera.SetCameraTarget(-lightPosition);

		glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_TRUE);
		glClear(GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_FRONT);
		draw_obj_depth();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_BACK);
		draw_obj();
		draw_cubemap();
		//draw_shadow();
	}

	void draw_shadow()
	{
		glUseProgram(shaderShadowView.program);
		glBindTexture(GL_TEXTURE_2D, depthTexture);

		glm::mat4 Matrix(1.0f);

		glUniformMatrix4fv(shaderShadowView.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

		static GLfloat plane_vertices[] = {
			-1.0, 1.0, 0.0,
			-0.5, 1.0, 0.0,
			-0.5, 0.5, 0.0,
			-1.0, 0.5, 0.0
		};

		static GLfloat plane_texcoords[] = {
			0.0, 1.0,
			1.0, 1.0,
			1.0, 0.0,
			0.0, 0.0
		};

		static GLuint plane_elements[] = {
			0, 1, 2,
			2, 3, 0
		};

		glEnableVertexAttribArray(POS_ATTRIB);
		glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, plane_vertices);

		glEnableVertexAttribArray(TEXTURE_ATTRIB);
		glVertexAttribPointer(TEXTURE_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, plane_texcoords);

		glDrawElements(GL_TRIANGLES, sizeof(plane_elements) / sizeof(*plane_elements), GL_UNSIGNED_INT, plane_elements);
	}

	void draw_obj()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shaderLightDepth.program);

		glm::mat4 biasMatrix(
			0.5, 0.0, 0.0, 0.0,
			0.0, 0.5, 0.0, 0.0,
			0.0, 0.0, 0.5, 0.0,
			0.5, 0.5, 0.5, 1.0
			);

		glm::mat4 projection, view, model;
		m_camera.GetMatrix(projection, view, model);

		glm::mat4 depthBiasMVP = biasMatrix * light_camera.GetMVPMatrix();

		glUniformMatrix4fv(shaderLightDepth.loc_model, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(shaderLightDepth.loc_view, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(shaderLightDepth.loc_projection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(shaderLightDepth.loc_u_DepthBiasMVP, 1, GL_FALSE, glm::value_ptr(depthBiasMVP));
		glUniform3fv(shaderLightDepth.loc_viewPos, 1, glm::value_ptr(m_camera.GetCameraPos()));

		glBindTexture(GL_TEXTURE_2D, depthTexture);

		Material material;
		Light light;

		material.ambient = glm::vec3(0.1745, 0.01175, 0.01175);
		material.diffuse = glm::vec3(0.61424, 0.04136, 0.04136);
		material.specular = glm::vec3(0.727811, 0.626959, 0.626959);
		material.shininess = 0.6f;

		glUniform3fv(shaderLightDepth.loc_material.ambient, 1, glm::value_ptr(material.ambient));
		glUniform3fv(shaderLightDepth.loc_material.diffuse, 1, glm::value_ptr(material.diffuse));
		glUniform1f(shaderLightDepth.loc_material.shininess, material.shininess);
		glUniform3fv(shaderLightDepth.loc_material.specular, 1, glm::value_ptr(material.specular));

		light.position = lightPosition;
		light.ambient = glm::vec3(1.0f, 1.0f, 1.0f);
		light.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
		light.specular = glm::vec3(0.0f, 0.0f, 0.0f);

		glUniform3fv(shaderLightDepth.loc_light.position, 1, glm::value_ptr(light.position));
		glUniform3fv(shaderLightDepth.loc_light.ambient, 1, glm::value_ptr(light.ambient));
		glUniform3fv(shaderLightDepth.loc_light.diffuse, 1, glm::value_ptr(light.diffuse));
		glUniform3fv(shaderLightDepth.loc_light.specular, 1, glm::value_ptr(light.specular));

		glBindVertexArray(modelOfFile.vaoObject);
		glDrawArrays(GL_TRIANGLES, 0, modelOfFile.vertices.size());
		glBindVertexArray(0);

		glm::mat4 model_annie = model * glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -2.0f, 2.5f)) * glm::scale(glm::mat4(1.0f), glm::vec3(2.5f));

		glUniformMatrix4fv(shaderLightDepth.loc_model, 1, GL_FALSE, glm::value_ptr(model_annie));
		glBindVertexArray(modelOfFileAnnie.vaoObject);
		glDrawArrays(GL_TRIANGLES, 0, modelOfFileAnnie.vertices.size());
		glBindVertexArray(0);

		glUniformMatrix4fv(shaderLightDepth.loc_model, 1, GL_FALSE, glm::value_ptr(model));
		material.ambient = glm::vec3(0.0, 0.0, 1.0);
		material.diffuse = glm::vec3(0.0, 0.0, 1.0);
		material.specular = glm::vec3(0.0, 0.0, 0.0);
		material.shininess = 1.0f;

		glUniform3fv(shaderLightDepth.loc_material.ambient, 1, glm::value_ptr(material.ambient));
		glUniform3fv(shaderLightDepth.loc_material.diffuse, 1, glm::value_ptr(material.diffuse));
		glUniform1f(shaderLightDepth.loc_material.shininess, material.shininess);
		glUniform3fv(shaderLightDepth.loc_material.specular, 1, glm::value_ptr(material.specular));

		light.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
		glUniform3fv(shaderLightDepth.loc_light.ambient, 1, glm::value_ptr(light.ambient));

		glEnableVertexAttribArray(POS_ATTRIB);
		glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, modelPlane.vertices.data());
		glEnableVertexAttribArray(NORMAL_ATTRIB);
		glVertexAttribPointer(NORMAL_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, modelPlane.normals.data());
		glDrawElements(GL_TRIANGLES, modelPlane.indexes.size(), GL_UNSIGNED_INT, modelPlane.indexes.data());

		glUseProgram(shaderSimpleColor.program);
		model = glm::translate(model, lightPosition) * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
		glUniformMatrix4fv(shaderSimpleColor.loc_uMVP, 1, GL_FALSE, glm::value_ptr(projection * view * model));
		glUniform3f(shaderSimpleColor.loc_objectColor, 1.0f, 1.0f, 1.0);

		glBindVertexArray(modelOfFileSphere.vaoObject);
		glDrawArrays(GL_TRIANGLES, 0, modelOfFileSphere.vertices.size());
		glBindVertexArray(0);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void draw_obj_depth()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderDepth.program);
		glm::mat4 projection, view, model;

		light_camera.GetMatrix(projection, view, model);

		glm::mat4 Matrix = projection * view * model;
		glUniformMatrix4fv(shaderDepth.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

		glBindVertexArray(modelOfFile.vaoObject);
		glDrawArrays(GL_TRIANGLES, 0, modelOfFile.vertices.size());
		glBindVertexArray(0);

		glm::mat4 model_annie = model * glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -2.0f, 2.5f)) * glm::scale(glm::mat4(1.0f), glm::vec3(2.5f));
		Matrix = projection * view * model_annie;
		glUniformMatrix4fv(shaderDepth.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

		glBindVertexArray(modelOfFileAnnie.vaoObject);
		glDrawArrays(GL_TRIANGLES, 0, modelOfFileAnnie.vertices.size());
		glBindVertexArray(0);
	}

	void draw_cubemap()
	{
		glDepthMask(GL_FALSE);
		glUseProgram(shaderSimpleCubeMap.program);
		glBindVertexArray(modelCube.vaoObject);
		glBindTexture(GL_TEXTURE_CUBE_MAP, c_textureID);

		glm::mat4 projection, view, model;

		m_camera.GetMatrix(projection, view, model);

		model = glm::scale(glm::mat4(1.0f), glm::vec3(40.0f)) * model;

		glm::mat4 Matrix = projection * view * model;
		glUniformMatrix4fv(shaderSimpleCubeMap.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

		glDrawArrays(GL_TRIANGLES, 0, modelCube.vertices.size() / 3);
		glBindVertexArray(0);
		glDepthMask(GL_TRUE);
	}

	GLuint FBOCreate(GLuint _Texture_depth)
	{
		GLuint FBO = 0;
		GLenum fboStatus;
		glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _Texture_depth, 0);
		if ((fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE)
		{
			DBG("glCheckFramebufferStatus error 0x%X\n", fboStatus);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return -1;
		}
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
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
		if (width != m_width || height != m_height)
		{
			if (depthTexture != -1)
				glDeleteTextures(1, &depthTexture);
			if (depthFBO != -1)
				glDeleteFramebuffers(1, &depthFBO);
			depthTexture = TextureCreateDepth(width, height);
			depthFBO = FBOCreate(depthTexture);
		}
		m_width = width;
		m_height = height;
		m_camera.SetViewport(x, y, width, height);
		light_camera.SetViewport(x, y, width, height);
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
	glm::vec3 camera;

	glm::vec3 axis_x;
	glm::vec3 axis_y;
	glm::vec3 axis_z;

	int m_width;
	int m_height;

	float angle = 0.0f;

	GLuint depthFBO = -1;
	GLuint c_textureID;

	GLuint depthTexture = -1;

	ModelOfFile modelOfFile;
	ShaderSimpleCubeMap shaderSimpleCubeMap;
	ModelCubeSkybox modelCube;
	ModelOfFile modelOfFileSphere;
	ModelOfFile modelOfFileAnnie;

	ModelPlane modelPlane;
	ShaderTexture shaderTexture;
	ShaderLightDepth shaderLightDepth;
	ShaderShadowView shaderShadowView;
	ShaderDepth shaderDepth;
	ShaderSimpleColor shaderSimpleColor;
	Camera m_camera;
	Camera light_camera;
};