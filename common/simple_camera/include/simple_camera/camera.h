#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>

using namespace gl;

enum Camera_Movement
{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

class Camera
{
public:
	glm::mat4 GetModelMatrix();

	glm::mat4 GetViewMatrix();

	glm::mat4 GetProjectionMatrix();

	void GetMatrix(glm::mat4 & projectionMatrix, glm::mat4 & viewMatrix, glm::mat4 & modelMatrix);

	glm::mat4 GetMVPMatrix();

	glm::vec3 GetCameraPos();

	void SetCameraPos(glm::vec3 _cameraPos);

	void SetCameraTarget(glm::vec3 _cameraTarget);

	void SetClipping(float near_clip_distance, float far_clip_distance);

	void SetViewport(int loc_x, int loc_y, int width, int height);

	void ProcessKeyboard(Camera_Movement direction, GLfloat deltaTime, bool moved = true);
private:
	glm::vec3 m_cameraPos = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 m_cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 m_cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	GLfloat MovementSpeed = 3.0f;

	float fovy = 45.0f;
	float aspect = 1.0f;
	float zNear = 0.5f;
	float zFar = 100.0f;

	glm::vec3 axis_x = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 axis_y = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 axis_z = glm::vec3(0.0f, 0.0f, 1.0f);

	float angle_x = 0.0f;
	float angle_y = 0.0f;
	float angle_z = 0.0f;
};