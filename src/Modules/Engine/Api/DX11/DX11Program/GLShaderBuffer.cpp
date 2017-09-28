﻿#include "Program/GLShaderBuffer.h"

GLShaderBuffer::GLShaderBuffer(ShaderType shader_type, GLuint program, GLint block_id, GLuint binding)
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

        VDesc& vdesc = m_vdesc[uni_name.data()];
        vdesc.size = -1;
        vdesc.offset = uni_offset;
    }
}

BufferProxy GLShaderBuffer::Uniform(const std::string & name)
{
    if (m_vdesc.count(name))
    {
        auto& desc = m_vdesc[name];
        return BufferProxy(m_data.data() + desc.offset, desc.size);
    }

    throw "name not found";
}

void GLShaderBuffer::Update()
{
    glBindBuffer(GL_UNIFORM_BUFFER, m_buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, m_data.size(), m_data.data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void GLShaderBuffer::SetOnPipeline()
{
    glBindBufferBase(GL_UNIFORM_BUFFER, m_binding, m_buffer);
}

GLuint GLShaderBuffer::CreateBuffer(size_t size)
{
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, buffer);
    glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    return buffer;
}
