#include "camera.h"

Camera::Camera()
	: axis_x(1.0f, 0.0f, 0.0f)
	, axis_y(0.0f, 1.0f, 0.0f)
	, axis_z(0.0f, 0.0f, 1.0f)
{
	camera_mode = CameraType::FREE;
	camera_up = glm::vec3(0, 1, 0);
	field_of_view = 45;
	camera_position_delta = glm::vec3(0, 0, 0);
	camera_scale = .1f;

	aspect = 1.0;
	near_clip = 0.1;
	far_clip = 100;

	camera_heading = 0.0f;
	camera_pitch = 0.0f;
}

Camera::~Camera()
{
}

void Camera::Update()
{
	camera_direction = glm::normalize(camera_look_at - camera_position);
	//need to set the matrix state. this is only important because lighting doesn't work if this isn't done
	glViewport(viewport_x, viewport_y, window_width, window_height);

	if (camera_mode == CameraType::ORTHO)
	{
		//our projection matrix will be an orthogonal one in this case
		//if the values are not floating point, this command does not work properly
		//need to multiply by aspect!!! (otherise will not scale properly)
		projection = glm::ortho(-1.5f * float(aspect), 1.5f * float(aspect), -1.5f, 1.5f, -10.0f, 10.f);
	}
	else if (camera_mode == CameraType::FREE)
	{
		projection = glm::perspective(field_of_view, aspect, near_clip, far_clip);
		//detmine axis for pitch rotation
		glm::vec3 axis = glm::cross(camera_direction, camera_up);
		//compute quaternion for pitch based on the camera pitch angle
		glm::quat pitch_quat = glm::angleAxis(camera_pitch, axis);
		//determine heading quaternion from the camera up vector and the heading angle
		glm::quat heading_quat = glm::angleAxis(camera_heading, camera_up);
		//add the two quaternions
		glm::quat temp = glm::cross(pitch_quat, heading_quat);
		temp = glm::normalize(temp);
		//update the direction from the quaternion
		camera_direction = glm::rotate(temp, camera_direction);
		//add the camera delta
		camera_position += camera_position_delta;
		//set the look at to be infront of the camera
		camera_look_at = camera_position + camera_direction * 1.0f;
		//damping for smooth camera
		camera_heading *= .5;
		camera_pitch *= .5;
		//camera_position_delta = camera_position_delta * .8f;
	}
	//compute the MVP
	view = glm::lookAt(camera_position, camera_look_at, camera_up);
	model = glm::mat4(1.0f);

	glm::mat4 animX = glm::rotate(glm::mat4(1.0f), angle_x, axis_x);
	glm::mat4 animY = glm::rotate(glm::mat4(1.0f), angle_y, axis_y);
	glm::mat4 animZ = glm::rotate(glm::mat4(1.0f), angle_z, axis_z);

	model = animX * animY * animZ * model;

	MVP = projection * view * model;
}

void Camera::SetClipping(double near_clip_distance, double far_clip_distance)
{
	near_clip = near_clip_distance;
	far_clip = far_clip_distance;
}

void Camera::SetMode(CameraType cam_mode)
{
	camera_mode = cam_mode;
	camera_up = glm::vec3(0, 1, 0);
}

void Camera::SetPosition(glm::vec3 pos)
{
	camera_position = pos;
}

void Camera::SetLookAt(glm::vec3 pos)
{
	camera_look_at = pos;
}

void Camera::SetFOV(double fov)
{
	field_of_view = fov;
}

void Camera::SetViewport(int loc_x, int loc_y, int width, int height)
{
	viewport_x = loc_x;
	viewport_y = loc_y;
	window_width = width;
	window_height = height;
	aspect = double(width) / double(height);
}
void Camera::Move(CameraDirection dir)
{
	if (camera_mode == CameraType::FREE)
	{
		switch (dir)
		{
		case CameraDirection::UP:
			camera_position_delta += camera_up * camera_scale;
			break;
		case CameraDirection::DOWN:
			camera_position_delta -= camera_up * camera_scale;
			break;
		case CameraDirection::LEFT:
			camera_position_delta -= glm::cross(camera_direction, camera_up) * camera_scale;
			break;
		case CameraDirection::RIGHT:
			camera_position_delta += glm::cross(camera_direction, camera_up) * camera_scale;
			break;
		case CameraDirection::FORWARD:
			camera_position_delta += camera_direction * camera_scale;
			break;
		case CameraDirection::BACK:
			camera_position_delta -= camera_direction * camera_scale;
			break;
		}
	}
}

CameraType Camera::GetMode()
{
	return camera_mode;
}

void Camera::GetViewport(int &loc_x, int &loc_y, int &width, int &height)
{
	loc_x = viewport_x;
	loc_y = viewport_y;
	width = window_width;
	height = window_height;
}

void Camera::GetMatricies(glm::mat4 &P, glm::mat4 &V, glm::mat4 &M)
{
	P = projection;
	V = view;
	M = model;
}