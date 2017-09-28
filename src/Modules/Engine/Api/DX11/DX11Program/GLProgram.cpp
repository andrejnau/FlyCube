#include "Program/GLProgram.h"
#include <Utilities/FileUtility.h>
#include <vector>
#include <fstream>

namespace ShaderUtility {

using ShaderVector = std::vector<std::pair<GLenum, std::string>>;

std::string GetShaderSource(const std::string &file)
{
    std::ifstream stream(GetAssetFullPath(file));
    std::string shader((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    return shader;
}

void CheckLinkStatus(GLuint program)
{
    std::string error;
    GLint link_status;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (!link_status)
    {
        GLint info_log_len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_len);
        std::vector<GLchar> info_log(info_log_len);
        glGetProgramInfoLog(program, info_log.size(), nullptr, info_log.data());
        error = info_log.data();
    }
}

void CheckCompileStatus(GLuint shader)
{
    std::string error;
    GLint compile_status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
    if (!compile_status)
    {
        GLint info_log_len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_len);
        std::vector<GLchar> info_log(info_log_len);
        glGetShaderInfoLog(shader, info_log.size(), nullptr, info_log.data());
        error = info_log.data();
    }
}

GLuint CreateShader(GLenum shaderType, const std::string &src)
{
    GLuint shader = glCreateShader(shaderType);
    const GLchar *source[] = { src.c_str() };
    glShaderSource(shader, 1, source, nullptr);
    glCompileShader(shader);
    CheckCompileStatus(shader);
    return shader;
}

GLuint CreateProgram(const ShaderVector &shaders)
{
    std::vector<GLuint> shadersId;
    for (auto & shader : shaders)
    {
        GLuint id = CreateShader(shader.first, shader.second.c_str());
        shadersId.push_back(id);
    }
    GLuint program = glCreateProgram();
    for (auto & id : shadersId)
    {
        glAttachShader(program, id);
    }
    glLinkProgram(program);
    CheckLinkStatus(program);
    return program;
}

} // ShaderUtility

GLProgram::GLProgram(const std::string & vertex, const std::string & fragment)
{
    std::string vertex_src = ShaderUtility::GetShaderSource(vertex);
    std::string fragment_src = ShaderUtility::GetShaderSource(fragment);

    ShaderUtility::ShaderVector shaders = {
        { GL_VERTEX_SHADER, vertex_src },
        { GL_FRAGMENT_SHADER, fragment_src }
    };

    m_program = ShaderUtility::CreateProgram(shaders);

    ParseVariable();
}

GLShaderBuffer & GLProgram::GetVSCBuffer(const std::string & name)
{
    auto it = m_vs_buffers.find(name);
    if (it != m_vs_buffers.end())
        return it->second;
    throw "name not found";
}

GLShaderBuffer & GLProgram::GetPSCBuffer(const std::string & name)
{
    auto it = m_ps_buffers.find(name);
    if (it != m_ps_buffers.end())
        return it->second;
    throw "name not found";
}

GLuint GLProgram::GetProgram() const
{
    return m_program;
}

UniformProxy GLProgram::Uniform(const std::string & name)
{
    return UniformProxy(GetLocation(name));
}

std::map<std::string, GLShaderBuffer>& GLProgram::GetCBuffersByType(ShaderType shader_type)
{
    if (shader_type == ShaderType::kVertex)
        return m_vs_buffers;
    else
        return m_ps_buffers;
}

ShaderType GLProgram::GetShaderType(GLuint block_id)
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

GLint GLProgram::GetLocation(const std::string & name)
{
    if (m_loc.count(name))
        return m_loc[name];
    return m_loc[name] = glGetUniformLocation(m_program, name.c_str());
}

void GLProgram::ParseVariable()
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

GLuint GLProgram::NextBinding()
{
    static GLuint cnt_blocks = 0;
    return cnt_blocks++;
}
