#pragma once
#include <platform.h>
#include <utilities.h>
#include <scenebase.h>
#include <state.h>
#include <math.h>
#include <string>
#include <vector>
#include <ctime>
#include <chrono>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "geometry.h"
#include "shaders.h"

class TestScene : public SceneBase
{
public:
    TestScene()
        : axis_x(1.0f, 0.0f, 0.0f)
        , axis_y(0.0f, 1.0f, 0.0f)
        , axis_z(0.0f, 0.0f, 1.0f)
        , modelSuzanne("model/suzanne.obj")
    {
    }

    virtual bool init()
    {
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glClearColor(0.365f, 0.54f, 0.66f, 1.0f);
        return true;
    }

    virtual void destroy()
    {
    }

    virtual void draw()
    {
        static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now(), end = std::chrono::system_clock::now();

        end = std::chrono::system_clock::now();
        int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        start = std::chrono::system_clock::now();
        static float angle = 0, angle_light = 0;
        angle += elapsed / 5000000.0f;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderLight.program);

        glm::vec3 lightPosition(2.0f * sin(angle_light), 2.0f * sin(angle_light), 2.0f  * sin(angle_light));
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
        glm::mat4 MatrixPV = projection * view;

        glUniformMatrix4fv(shaderLight.loc_VP, 1, GL_FALSE, glm::value_ptr(MatrixPV));
        glUniformMatrix4fv(shaderLight.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

        glUniform3fv(shaderLight.loc_lightPosition, 1, glm::value_ptr(lightPosition));
        glUniform3fv(shaderLight.loc_camera, 1, glm::value_ptr(camera));

        glBindVertexArray(modelSuzanne.vaoObject);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)modelSuzanne.vertices.size());
        glBindVertexArray(0);
    }

    virtual void resize(int x, int y, int width, int height)
    {
        glViewport(x, y, width, height);
        m_width = width;
        m_height = height;
    }
private:
    glm::vec3 axis_x;
    glm::vec3 axis_y;
    glm::vec3 axis_z;

    int m_width;
    int m_height;
    ModelOfFile modelSuzanne;
    ShaderLight shaderLight;
};
