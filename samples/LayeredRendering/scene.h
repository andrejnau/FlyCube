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
	{
	}
	
	virtual bool init()
	{
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.365f, 0.54f, 0.66f, 1.0f);

		c_textureID = loadTexture();

		c_texture3d = initGLTexture3D(tex_width, tex_height, 32);

		c_fbo = initLayeredFBO(c_texture3d);
		return true;
	}

	void CopyToTexture2D(GLuint texture_in, GLuint slice, GLuint texture_out)
	{
		GLuint read_fbo;
		glGenFramebuffers(1, &read_fbo);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fbo);
		glFramebufferTexture3D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, texture_in, 0, slice);
		assert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		glBindTexture(GL_TEXTURE_2D, texture_out);

		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,0, 0, tex_width, tex_height);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void LayeredDraw()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, c_fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shaderLayered.program);
		glBindVertexArray(modelTriangle.vaoObject);
		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)modelTriangle.vertices.size());
		glBindVertexArray(0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	virtual void draw()
	{
		LayeredDraw();

		static int slice = 0;
		CopyToTexture2D(c_texture3d, slice , c_textureID);
		if (rand() % 100 == 42)
			slice = (slice + 1) % 32;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderTexture.program);

		glm::mat4 model(1.0f);
		glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		float aspect = float(m_width) / float(m_height);
		glm::mat4 projection = glm::ortho(-1.0f * aspect, 1.0f * aspect, -1.0f / aspect, 1.0f / aspect, 0.5f, 100.0f);
		glm::mat4 Matrix = projection * view * model;

		glUniformMatrix4fv(shaderTexture.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));
		glBindTexture(GL_TEXTURE_2D, c_textureID);

		glBindVertexArray(modelForTexture.vaoObject);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelForTexture.vboIndexe);

		glDrawElements(GL_TRIANGLES, (GLsizei)modelForTexture.indexes.size(), GL_UNSIGNED_INT, (void*)0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	GLuint initLayeredFBO(GLuint texture_ID)
	{
		GLuint c_fbo;
		glGenFramebuffers(1, &c_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, c_fbo);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_ID, 0);
		assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return c_fbo;
	}

	GLuint initGLTexture3D(int width, int height, int layerCount)
	{
		GLuint textureID;
		glGenTextures(1, &textureID);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, textureID);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, width, height, layerCount, 0,GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		return textureID;
	}

	GLuint initGLTexture2D(int width, int height)
	{
		GLuint textureID;
		glGenTextures(1, &textureID);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		return textureID;
	}

	GLuint loadTexture()
	{
		GLuint textureID;
		glGenTextures(1, &textureID);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);

		std::string texture_path = std::string(PROJECT_RESOURCE_DIR "texture/test_texture.png");

		int width, height, channels;
		unsigned char * image = SOIL_load_image(texture_path.c_str(), &width, &height, &channels, SOIL_LOAD_RGBA);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		SOIL_free_image_data(image);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_TEXTURE_BORDER_COLOR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_TEXTURE_BORDER_COLOR);
		glBindTexture(GL_TEXTURE_2D, 0);
		GLenum err = glGetError();
		return textureID;
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
	int m_width;
	int m_height;

	GLuint tex_width = 128;
	GLuint tex_height = 128;

	ShaderTexture shaderTexture;
	ShaderLayered shaderLayered;
	ModelForTexture modelForTexture;
	ModelTriangle modelTriangle;

	GLuint c_textureID;
	GLuint c_texture3d;
	GLuint c_fbo;
};