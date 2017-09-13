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
        //static_assert(false, "not implemented");
    }

private:
    GLint m_loc;
};

class UniformProxyBuffer
{
public:
    UniformProxyBuffer(char* data, size_t size)
        : m_data(data)
        , m_size(size)
    {
    }

    template<typename T>
    void operator=(const T& value)
    {
        //assert(sizeof(T) == m_size);
        *reinterpret_cast<T*>(m_data) = value;
    }

private:
    char* m_data;
    size_t m_size;
};

enum class ShaderType
{
    kVertex,
    kPixel
};

class CBuffer
{
public:
    GLuint CreateBuffer(size_t size)
    {
        GLuint buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, buffer);
        glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        return buffer;
    }

    CBuffer(ShaderType shader_type, GLuint program, GLint block_id, GLuint binding)
        : m_shader_type(shader_type)
        , m_program(program)
        , m_uniform_block(block_id)
        , m_binding(binding)
    {
        GLint block_size;
        glGetActiveUniformBlockiv(m_program, m_uniform_block, GL_UNIFORM_BLOCK_DATA_SIZE, &block_size);

        m_buffer = CreateBuffer(block_size);
        m_data.resize(block_size);
        glUniformBlockBinding(m_program, m_uniform_block, m_binding);

        GLint num_uni;
        glGetActiveUniformBlockiv(m_program, m_uniform_block, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &num_uni);

        std::vector<GLint> active_indexes(num_uni);
        glGetActiveUniformBlockiv(m_program, m_uniform_block, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, active_indexes.data());

        for (int uni_id = 0; uni_id < num_uni; ++uni_id)
        {
            GLuint uni_location = active_indexes[uni_id];

            GLint uni_name_len;
            glGetActiveUniformsiv(m_program, (GLsizei)1, &uni_location, GL_UNIFORM_NAME_LENGTH, &uni_name_len);

            std::vector<GLchar> uni_name(uni_name_len);
            glGetActiveUniformName(m_program, uni_location, uni_name.size(), nullptr, uni_name.data());

            GLint uni_offset;
            glGetActiveUniformsiv(m_program, (GLsizei)1, &uni_location, GL_UNIFORM_OFFSET, &uni_offset);

            GLint uni_size;
            glGetActiveUniformsiv(m_program, (GLsizei)1, &uni_location, GL_UNIFORM_SIZE, &uni_size);

            VDesc& vdesc = m_vdesc[uni_name.data()];
            vdesc.size = uni_size;
            vdesc.offset = uni_offset;
        }
    }

    UniformProxyBuffer Uniform(const std::string& name)
    {
        if (m_vdesc.count(name))
        {
            auto& desc = m_vdesc[name];
            return UniformProxyBuffer(m_data.data() + desc.offset, desc.size);
        }

        throw "name not found";
    }

    void Update()
    {
        glBindBuffer(GL_UNIFORM_BUFFER, m_buffer);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, m_data.size(), m_data.data());
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void SetOnPipeline()
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, m_binding, m_buffer);
    }

private:

    struct VDesc
    {
        size_t size;
        size_t offset;
    };

    std::map<std::string, VDesc> m_vdesc;
    GLuint m_program;
    GLint m_uniform_block;
    GLuint m_binding;
    ShaderType m_shader_type;
    GLuint m_buffer;
    std::vector<char> m_data;
};

class GLShader
{
public:
    std::map<std::string, CBuffer> m_vs_buffers;
    std::map<std::string, CBuffer> m_ps_buffers;

    GLuint NextBinding()
    {
        static GLuint cnt_blocks = 0;
        return cnt_blocks++;
    }

    std::map<std::string, CBuffer>& GetCBuffersByType(ShaderType shader_type)
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

    CBuffer& GetVSCBuffer(const std::string& name)
    {
        auto it = m_vs_buffers.find(name);
        if (it != m_vs_buffers.end())
            return it->second;
        throw "name not found";
    }

    CBuffer& GetPSCBuffer(const std::string& name)
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
            buffers.emplace(block_name.data(), CBuffer(shader_type, m_program, block_id, NextBinding()));
        }
    }

    GLShader(const std::string& vertex, const std::string& fragment)
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
