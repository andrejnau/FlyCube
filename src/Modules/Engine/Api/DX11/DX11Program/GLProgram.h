#pragma once
#include "Program/GLShaderBuffer.h"
#include "Program/GLUniformProxy.h"
#include <glLoadGen/gl.h>
#include <string>
#include <map>

class GLProgram
{
public:
    GLProgram(const std::string& vertex, const std::string& fragment);
    GLShaderBuffer& GetVSCBuffer(const std::string& name);
    GLShaderBuffer& GetPSCBuffer(const std::string& name);
    GLuint GetProgram() const;
    UniformProxy Uniform(const std::string& name);

private:
    std::map<std::string, GLShaderBuffer>& GetCBuffersByType(ShaderType shader_type);
    ShaderType GetShaderType(GLuint block_id);
    GLint GetLocation(const std::string& name);
    void ParseVariable();
    GLuint NextBinding();

    std::map<std::string, GLShaderBuffer> m_vs_buffers;
    std::map<std::string, GLShaderBuffer> m_ps_buffers;
    std::map<std::string, GLint> m_loc;
    GLuint m_program;
};
