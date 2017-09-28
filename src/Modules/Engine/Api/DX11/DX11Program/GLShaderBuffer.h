#pragma once

#include "Program/IShaderBuffer.h"
#include "Program/ShaderType.h"
#include <glLoadGen/gl.h>
#include <string>
#include <vector>
#include <map>

class GLShaderBuffer : public IShaderBuffer
{
public:
    GLShaderBuffer(ShaderType shader_type, GLuint program, GLint block_id, GLuint binding);
    virtual BufferProxy Uniform(const std::string& name) override;
    void Update();
    void SetOnPipeline();

private:
    GLuint CreateBuffer(size_t size);

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
