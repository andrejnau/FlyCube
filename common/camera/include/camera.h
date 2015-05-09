//the basis is https://github.com/hmazhar/moderngl_camera
#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <gl_core.h>

enum class CameraType
{
	ORTHO, FREE
};

enum class CameraDirection
{
	UP, DOWN, LEFT, RIGHT, FORWARD, BACK
};

class Camera
{
public:
	Camera();
	~Camera();

	void Update();

	//Given a specific moving direction, the camera will be moved in the appropriate direction
	//For a spherical camera this will be around the look_at point
	//For a free camera a delta will be computed for the direction of movement.
	void Move(CameraDirection dir);

	//Changes the camera mode, only three valid modes, Ortho, Free, and Spherical
	void SetMode(CameraType cam_mode);

	//Set the position of the camera
	void SetPosition(glm::vec3 pos);

	//Set's the look at point for the camera
	void SetLookAt(glm::vec3 pos);

	//Changes the Field of View (FOV) for the camera
	void SetFOV(double fov);

	//Change the viewport location and size
	void SetViewport(int loc_x, int loc_y, int width, int height);

	void SetPos(int button, int state, int x, int y);

	void SetClipping(double near_clip_distance, double far_clip_distance);

	//Getting Functions
	CameraType GetMode();

	void GetViewport(int &loc_x, int &loc_y, int &width, int &height);

	void GetMatricies(glm::mat4 &P, glm::mat4 &V, glm::mat4 &M);

	float angle_x = 0.0f;
	float angle_y = 0.0f;
	float angle_z = 0.0f;

private:
	CameraType camera_mode;

	int viewport_x;
	int viewport_y;

	int window_width;
	int window_height;

	double aspect;
	double field_of_view;
	double near_clip;
	double far_clip;

	float camera_scale;
	float camera_heading;
	float camera_pitch;

	glm::vec3 camera_position;
	glm::vec3 camera_position_delta;
	glm::vec3 camera_look_at;
	glm::vec3 camera_direction;

	glm::vec3 camera_up;
	glm::vec3 mouse_position;

	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 model;
	glm::mat4 MVP;

	glm::vec3 axis_x;
	glm::vec3 axis_y;
	glm::vec3 axis_z;
};