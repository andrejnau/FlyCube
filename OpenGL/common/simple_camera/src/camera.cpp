#include <simple_camera/camera.h>

glm::mat4 Camera::GetModelMatrix()
{
    glm::mat4 animX = glm::rotate(glm::mat4(1.0f), angle_x, axis_x);
    glm::mat4 animY = glm::rotate(glm::mat4(1.0f), angle_y, axis_y);
    glm::mat4 animZ = glm::rotate(glm::mat4(1.0f), angle_z, axis_z);

    glm::mat4 model = animX * animY * animZ;
    return model;
}

glm::mat4 Camera::GetViewMatrix()
{
    return glm::lookAt(m_cameraPos, m_cameraTarget, m_cameraUp);
}

glm::mat4 Camera::GetProjectionMatrix()
{
    return glm::perspective(fovy, aspect, zNear, zFar);
}

void Camera::GetMatrix(glm::mat4 & projectionMatrix, glm::mat4 & viewMatrix, glm::mat4 & modelMatrix)
{
    projectionMatrix = GetProjectionMatrix();
    viewMatrix = GetViewMatrix();
    modelMatrix = GetModelMatrix();
}

glm::mat4 Camera::GetMVPMatrix()
{
    return GetProjectionMatrix() * GetViewMatrix() * GetModelMatrix();
}

glm::vec3 Camera::GetCameraPos()
{
    return m_cameraPos;
}

void Camera::SetCameraPos(glm::vec3 _cameraPos)
{
    m_cameraPos = _cameraPos;
}

void Camera::SetCameraTarget(glm::vec3 _cameraTarget)
{
    m_cameraTarget = _cameraTarget;
}

void Camera::SetClipping(float near_clip_distance, float far_clip_distance)
{
    zNear = near_clip_distance;
    zFar = far_clip_distance;
}

void Camera::SetViewport(int loc_x, int loc_y, int width, int height)
{
    aspect = float(width) / float(height);
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime, bool moved)
{
    glm::vec3 cameraDirection = glm::normalize(m_cameraTarget - m_cameraPos);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraRight = glm::normalize(glm::cross(cameraUp, cameraDirection));

    float velocity = MovementSpeed * deltaTime;
    float angle_eps = deltaTime;
    if (direction == FORWARD)
    {
        if (moved)
        {
            m_cameraPos += cameraDirection * velocity;
            m_cameraTarget += cameraDirection * velocity;
        }
        else
        {
            angle_x += angle_eps;
        }
    }
    else if (direction == BACKWARD)
    {
        if (moved)
        {
            m_cameraPos -= cameraDirection * velocity;
            m_cameraTarget -= cameraDirection * velocity;
        }
        else
        {
            angle_x -= angle_eps;
        }
    }
    else if (direction == LEFT)
    {
        if (moved)
        {
            m_cameraPos += cameraRight * velocity;
            if (moved)
                m_cameraTarget += cameraRight * velocity;
        }
        else
        {
            angle_y += angle_eps;
        }
    }
    else if (direction == RIGHT)
    {
        if (moved)
        {
            m_cameraPos -= cameraRight * velocity;
            m_cameraTarget -= cameraRight * velocity;
        }
        else
        {
            angle_y -= angle_eps;
        }
    }
    else if (direction == UP)
    {
        if (moved)
        {
            m_cameraPos += cameraUp * velocity;
            m_cameraTarget += cameraUp * velocity;
        }
        else
        {
            angle_z += angle_eps;
        }
    }

    else if (direction == DOWN)
    {
        if (moved)
        {
            m_cameraPos -= cameraUp * velocity;
            m_cameraTarget -= cameraUp * velocity;
        }
        else
        {
            angle_z -= angle_eps;
        }
    }
}