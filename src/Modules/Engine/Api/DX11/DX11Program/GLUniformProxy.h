#pragma once

#include <glLoadGen/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class UniformProxy
{
public:
    UniformProxy(GLint loc)
        : m_loc(loc)
    {
    }

    void operator=(const glm::mat4& x)
    {
        glUniformMatrix4fv(m_loc, 1, GL_FALSE, glm::value_ptr(x));
    }

    void operator=(const glm::vec3& x)
    {
        glUniform3f(m_loc, x.x, x.y, x.z);
    }

    void operator=(const std::vector<glm::vec3>& x)
    {
        glUniform3fv(m_loc, (GLsizei)x.size(), glm::value_ptr(x.front()));
    }

    void operator=(const GLint& x)
    {
        glUniform1i(m_loc, x);
    }

    void operator=(const float& x)
    {
        glUniform1f(m_loc, x);
    }

private:
    GLint m_loc;
};
