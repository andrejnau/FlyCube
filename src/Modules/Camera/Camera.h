#pragma once

#include <glm/glm.hpp>
#include <vector>

enum class CameraMovement
{
    kForward,
    kBackward,
    kLeft,
    kRight,
    kUp,
    kDown,
};

const float kYaw = -90.0f;
const float kPitch = 0.0f;
const float kSpeed = 5.0f;
const float kSensitivty = 0.25f;
const float kZoom = 45.0f;

class Camera
{
public:
    // Camera Attributes
    glm::vec3 position_;
    glm::vec3 front_;
    glm::vec3 up_;
    glm::vec3 right_;
    glm::vec3 world_up_;
    // Eular Angles
    float yaw_;
    float pitch_;
    // Camera options
    float movement_speed_;
    float mouse_sensitivity_;
    float zoom_;

    float fovy_ = 45.0f;
    float aspect_ = 1.0f;
    float z_near_ = 0.1f;
    float z_far_ = 1024.0f;

    // Constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = kYaw, float pitch = kPitch);

    void SetViewport(int width, int height);

    void SetCameraPos(glm::vec3 camera_pos);

    glm::vec3 GetCameraPos() const;

    glm::mat4 GetProjectionMatrix() const;

    glm::mat4 GetViewMatrix() const;

    glm::mat4 GetModelMatrix() const;

    void GetMatrix(glm::mat4 & projection_matrix, glm::mat4 & view_matrix, glm::mat4 & model_matrix) const;

    glm::mat4 GetMVPMatrix() const;

    // Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(CameraMovement direction, float delta_time);

    // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrain_pitch = true);

    // Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset);

private:
    // Calculates the front vector from the Camera's (updated) Eular Angles
    void updateCameraVectors();
};