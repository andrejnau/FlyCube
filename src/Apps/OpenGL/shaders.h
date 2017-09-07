#pragma once

#include <platform.h>
#include <utilities.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include <map>

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

    template<typename T>
    void operator=(const T& x)
    {
        static_assert(false, "not implemented");
    }

private:
    GLint m_loc;
};

class GLShader
{
public:
    GLShader(const std::string& vertex, const std::string& fragment)
    {
        std::string vertex_src = GetShaderSource(vertex);
        std::string fragment_src = GetShaderSource(fragment);

        ShaderVector shaders = {
            { GL_VERTEX_SHADER, vertex_src },
            { GL_FRAGMENT_SHADER, fragment_src }
        };

        m_program = CreateProgram(shaders);
    }

    GLuint GetProgram() const
    {
        return m_program;
    }

    UniformProxy Uniform(const std::string& name)
    {
        return UniformProxy(GetLocation(name));
    }

private:
    GLint GetLocation(const std::string& name)
    {
        if (m_loc.count(name))
            return m_loc[name];
        return m_loc[name] = glGetUniformLocation(m_program, name.c_str());
    }

    std::map<std::string, GLint> m_loc;
    GLuint m_program;
};
