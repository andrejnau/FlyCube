#include <simple_camera/camera.h>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : front_(glm::vec3(0.0f, 0.0f, -1.0f)),
      movement_speed_(kSpeed),
      mouse_sensitivity_(kSensitivty),
      zoom_(kZoom)
{
    position_ = position;
    world_up_ = up;
    yaw_ = yaw;
    pitch_ = pitch;
    updateCameraVectors();
}

void Camera::SetViewport(int loc_x, int loc_y, int width, int height)
{
    aspect_ = float(width) / float(height);
}

void Camera::SetCameraPos(glm::vec3 camera_pos)
{
    position_ = camera_pos;
    updateCameraVectors();
}

glm::vec3 Camera::GetCameraPos() const
{
    return position_;
}

glm::mat4 Camera::GetProjectionMatrix() const
{
    return glm::perspective(fovy_, aspect_, z_near_, z_far_);
}

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(position_, position_ + front_, up_);
}

glm::mat4 Camera::GetModelMatrix() const
{
    return glm::mat4(1.0);
}

void Camera::GetMatrix(glm::mat4 &projection_matrix, glm::mat4 &view_matrix, glm::mat4 &model_matrix) const
{
    projection_matrix = GetProjectionMatrix();
    view_matrix = GetViewMatrix();
    model_matrix = GetModelMatrix();
}

glm::mat4 Camera::GetMVPMatrix() const
{
    return GetProjectionMatrix() * GetViewMatrix() * GetModelMatrix();
}

// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
void Camera::ProcessKeyboard(CameraMovement direction, float delta_time)
{
    float velocity = movement_speed_ * delta_time;
    if (direction == CameraMovement::kForward)
        position_ += front_ * velocity;
    if (direction == CameraMovement::kBackward)
        position_ -= front_ * velocity;
    if (direction == CameraMovement::kLeft)
        position_ -= right_ * velocity;
    if (direction == CameraMovement::kRight)
        position_ += right_ * velocity;
    if (direction == CameraMovement::kUp)
        position_ += up_ * velocity;
    if (direction == CameraMovement::kDown)
        position_ -= up_ * velocity;
}

// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrain_pitch)
{
    xoffset *= mouse_sensitivity_;
    yoffset *= mouse_sensitivity_;

    yaw_ += xoffset;
    pitch_ += yoffset;

    // Make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrain_pitch)
    {
        if (pitch_ > 89.0f)
            pitch_ = 89.0f;
        if (pitch_ < -89.0f)
            pitch_ = -89.0f;
    }

    // Update Front, Right and Up Vectors using the updated Eular angles
    updateCameraVectors();
}

// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
void Camera::ProcessMouseScroll(float yoffset)
{
    if (zoom_ >= 1.0f && zoom_ <= 45.0f)
        zoom_ -= yoffset;
    if (zoom_ <= 1.0f)
        zoom_ = 1.0f;
    if (zoom_ >= 45.0f)
        zoom_ = 45.0f;
}

// Calculates the front vector from the Camera's (updated) Eular Angles
inline void Camera::updateCameraVectors()
{
    // Calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front.y = sin(glm::radians(pitch_));
    front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front_ = glm::normalize(front);
    // Also re-calculate the Right and Up vector
    right_ = glm::normalize(glm::cross(front_, world_up_));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    up_ = glm::normalize(glm::cross(right_, front_));
}
