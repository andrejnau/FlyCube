#pragma once

#include <platform.h>
#include <utilities.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include <map>

#include "GLShaderBuffer.h"
#include "GLUniformProxy.h"

class GLProgram
{
public:
    std::map<std::string, GLShaderBuffer> m_vs_buffers;
    std::map<std::string, GLShaderBuffer> m_ps_buffers;

    GLuint NextBinding()
    {
        static GLuint cnt_blocks = 0;
        return cnt_blocks++;
    }

    std::map<std::string, GLShaderBuffer>& GetCBuffersByType(ShaderType shader_type)
    {
        if (shader_type == ShaderType::kVertex)
            return m_vs_buffers;
        else
            return m_ps_buffers;
    }

    ShaderType GetShaderType(GLuint block_id)
    {
        GLint is_vertex;
        glGetActiveUniformBlockiv(m_program, block_id, GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER, &is_vertex);
        if (is_vertex)
            return ShaderType::kVertex;

        GLint is_fragment;
        glGetActiveUniformBlockiv(m_program, block_id, GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER, &is_fragment);
        if (is_fragment)
            return ShaderType::kPixel;

        throw "unknown shader type";
    }

    GLShaderBuffer& GetVSCBuffer(const std::string& name)
    {
        auto it = m_vs_buffers.find(name);
        if (it != m_vs_buffers.end())
            return it->second;
        throw "name not found";
    }

    GLShaderBuffer& GetPSCBuffer(const std::string& name)
    {
        auto it = m_ps_buffers.find(name);
        if (it != m_ps_buffers.end())
            return it->second;
        throw "name not found";
    }

    void ParseVariable()
    {
        GLint num_blocks;
        glGetProgramiv(m_program, GL_ACTIVE_UNIFORM_BLOCKS, &num_blocks);
        for (GLuint block_id = 0; block_id < num_blocks; ++block_id)
        {
            GLint block_name_len;
            glGetActiveUniformBlockiv(m_program, block_id, GL_UNIFORM_BLOCK_NAME_LENGTH, &block_name_len);

            std::vector<GLchar> block_name(block_name_len);
            glGetActiveUniformBlockName(m_program, block_id, block_name.size(), nullptr, block_name.data());

            ShaderType shader_type = GetShaderType(block_id);
            auto& buffers = GetCBuffersByType(shader_type);
            buffers.emplace(block_name.data(), GLShaderBuffer(shader_type, m_program, block_id, NextBinding()));
        }
    }

    GLProgram(const std::string& vertex, const std::string& fragment)
    {
        std::string vertex_src = GetShaderSource(vertex);
        std::string fragment_src = GetShaderSource(fragment);

        ShaderVector shaders = {
            { GL_VERTEX_SHADER, vertex_src },
            { GL_FRAGMENT_SHADER, fragment_src }
        };

        m_program = CreateProgram(shaders);

        ParseVariable();
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
