#include "GLProgramApi.h"

#include <vector>
#include <utility>
#include <Context/GLResource.h>
#include <Context/VKView.h>

#include <glLoadGen/gl.h>

namespace ShaderUtility
{
    using ShaderVector = std::vector<std::pair<GLenum, std::string>>;

    inline std::string GetAssetFullPath(const std::string &assetName)
    {
        std::string path = std::string(ASSETS_PATH) + assetName;
        if (!std::ifstream(path).good())
            path = "assets/" + assetName;
        return path;
    }

    inline std::string GetShaderSource(const std::string &file)
    {
        std::ifstream stream(GetAssetFullPath(file));
        std::string shader((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        return shader;
    }

    inline void CheckLink(GLuint program)
    {
        std::string err;
        GLint link_ok;
        glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
        if (!link_ok)
        {
            GLint infoLogLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
            std::vector<GLchar> info_log(infoLogLength);
            glGetProgramInfoLog(program, info_log.size(), nullptr, info_log.data());
            err = info_log.data();
        }
    }

    inline void CheckValidate(GLuint program)
    {
        std::string err;
        GLint link_ok;
        glGetProgramiv(program, GL_VALIDATE_STATUS, &link_ok);
        if (!link_ok)
        {
            GLint infoLogLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
            std::vector<GLchar> info_log(infoLogLength);
            glGetProgramInfoLog(program, info_log.size(), nullptr, info_log.data());
            err = info_log.data();
        }
    }

    inline void CheckCompile(GLuint shader)
    {
        std::string err;
        GLint link_ok;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &link_ok);
        if (!link_ok)
        {
            GLint infoLogLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
            std::vector<GLchar> info_log(infoLogLength);
            glGetShaderInfoLog(shader, info_log.size(), nullptr, info_log.data());
            err = info_log.data();
        }
    }

    inline GLuint CreateShader(GLenum shaderType, const std::string &src)
    {
        GLuint shader = glCreateShader(shaderType);
        const GLchar *source[] = { src.c_str() };
        glShaderSource(shader, 1, source, nullptr);
        glCompileShader(shader);
        CheckCompile(shader);
        return shader;
    }

    inline GLuint CreateProgram(const ShaderVector &shaders)
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
        CheckLink(program); 

        return program;
    }
} // ShaderUtility

GLProgramApi::GLProgramApi(GLContext& context)
    : m_context(context)
{
    glCreateFramebuffers(1, &m_framebuffer);
    glCreateVertexArrays(1, &m_vao);

    m_samplers.insert("SPIRV_Cross_DummySampler");
}

void GLProgramApi::SetMaxEvents(size_t count)
{
}

GLenum GetGLProgTagret(ShaderType type)
{
    switch (type)
    {
    case ShaderType::kVertex:
        return GL_VERTEX_SHADER;
    case ShaderType::kPixel:
        return GL_FRAGMENT_SHADER;
    case ShaderType::kCompute:
        return GL_COMPUTE_SHADER;
    case ShaderType::kGeometry:
        return GL_GEOMETRY_SHADER;
    }
    return 0;
}

void replace(std::string& str, const std::string& from, const std::string& to)
{
    if (from.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

void GLProgramApi::LinkProgram()
{
    ShaderUtility::ShaderVector shaders;

    for (auto shader : m_src)
    {
        shaders.push_back({
            GetGLProgTagret(shader.first),
            shader.second });
    }

    m_program = ShaderUtility::CreateProgram(shaders);

    ParseShaders(); 
    ParseVariable();
    ParseSSBO();
    ParseShadersOuput();
}

void GLProgramApi::UseProgram()
{
    glUseProgram(m_program);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer);
    glDrawBuffers(m_attachments.size(), m_attachments.data());
   
    m_context.UseProgram(*this);
}

void GLProgramApi::ApplyBindings()
{
    glBindVertexArray(m_vao);

    glValidateProgram(m_program);
    ShaderUtility::CheckValidate(m_program);

    for (auto &x : m_cbv_layout)
    {
        BufferLayout& buffer = x.second;
        auto& res = m_cbv_buffer[x.first];
        if (buffer.SyncData())
        {
            m_context.UpdateSubresource(res, 0, buffer.GetBuffer().data(), buffer.GetBuffer().size(), 0);
        }

        GLResource& gl_res = static_cast<GLResource&>(*res);

        std::string name = m_cbv_name[x.first];
        if (name == "$Globals")
            name = "_Global";

        auto it = m_cbv_bindings.find(name);
        assert(it != m_cbv_bindings.end());
        glBindBufferBase(GL_UNIFORM_BUFFER, it->second, gl_res.buffer);
    }
}

static std::vector<uint8_t> readFile(const char* filename)
{
    // open the file:
    std::streampos fileSize;
    std::ifstream file(filename, std::ios::binary);

    // get its size:
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // read the data:
    std::vector<unsigned char> fileData(fileSize);
    file.read((char*)&fileData[0], fileSize);
    return fileData;
}

static std::string get_tmp_file(const std::string& prefix)
{
    char tmpdir[MAX_PATH] = {};
    GetTempPathA(MAX_PATH, tmpdir);
    return tmpdir + prefix;
}

std::vector<uint8_t> GLProgramApi::hlsl2spirv(const ShaderBase& shader)
{
    std::string shader_type;
    switch (shader.type)
    {
    case ShaderType::kPixel:
        shader_type = "frag";
        break;
    case ShaderType::kVertex:
        shader_type = "vert";
        break;
    case ShaderType::kGeometry:
        shader_type = "geom";
        break;
    case ShaderType::kCompute:
        shader_type = "comp";
        break;
    }

    std::string glsl_name = shader.shader_path;
    glsl_name = glsl_name.substr(glsl_name.find_last_of("\\/")+1);
    glsl_name = glsl_name.replace(glsl_name.find(".hlsl"), 5, ".glsl");
    std::string spirv_path = get_tmp_file("SponzaApp.spirv");
    std::string glsl_path = get_tmp_file("SponzaApp.glsl");

    std::string cmd = "C:\\VulkanSDK\\1.1.82.1\\Bin\\glslangValidator.exe";
  ///  cmd += " --auto-map-bindings --hlsl-iomap ";
    cmd += " --invert-y ";
    cmd += " -g ";
    cmd += " -e ";
    cmd += shader.entrypoint;
    cmd += " -S ";
    cmd += shader_type;
    cmd += " -V -D ";
    cmd += GetAssetFullPath(shader.shader_path);
    cmd += " -o ";
    cmd += spirv_path;

    for (auto &x : shader.define)
    {
        cmd += " -D" + x.first + "=" + x.second;
    }

    DeleteFileA(spirv_path.c_str());
    system(cmd.c_str());

    cmd = "C:\\VulkanSDK\\1.1.82.1\\Bin\\spirv-cross.exe";
    cmd += " --version 450 ";
    cmd += spirv_path;
    cmd += " --output " + glsl_path;

    DeleteFileA(glsl_path.c_str());
    system(cmd.c_str());

    auto res = readFile(glsl_path.c_str());

    DeleteFileA(glsl_path.c_str());
    DeleteFileA(spirv_path.c_str());
    return res;
}

void GLProgramApi::CompileShader(const ShaderBase& shader)
{
    auto spirv = hlsl2spirv(shader);
    std::string source(spirv.begin(), spirv.end());
    m_src[shader.type] = source;
}

void GLProgramApi::ParseShader(ShaderType shader_type, const std::vector<uint32_t>& spirv_binary, std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
}

GLenum AttribComponentType(GLenum type)
{
    switch (type)
    {
    case GL_BOOL:
    case GL_BOOL_VEC2:
    case GL_BOOL_VEC3:
    case GL_BOOL_VEC4:
        return GL_BOOL;
    case GL_FLOAT:
    case GL_FLOAT_VEC2:
    case GL_FLOAT_VEC3:
    case GL_FLOAT_VEC4:
        return GL_FLOAT;
    default:
        return type;
    }
}

int AttribColumnCount(GLenum type)
{
    switch (type)
    {
    case GL_BOOL_VEC2:
    case GL_FLOAT_VEC2:
    case GL_INT_VEC2:
        return 2;
    case GL_INT_VEC3:
    case GL_FLOAT_VEC3:
    case GL_BOOL_VEC3:
        return 3;
    case GL_BOOL_VEC4:
    case GL_FLOAT_VEC4:
    case GL_INT_VEC4:
        return 4;
    default:
        return 1;
    }

    return 0;
}


GLuint NextBinding()
{
    static GLuint cnt_blocks = 0;
    return cnt_blocks++;
}

GLuint NextSSBOBinding()
{
    static GLuint cnt_blocks = 0;
    return cnt_blocks++;
}

void GLProgramApi::ParseVariable()
{
    GLint num_blocks;
    glGetProgramiv(m_program, GL_ACTIVE_UNIFORM_BLOCKS, &num_blocks);
    for (GLuint block_id = 0; block_id < num_blocks; ++block_id)
    {
        GLint block_name_len;
        glGetActiveUniformBlockiv(m_program, block_id, GL_UNIFORM_BLOCK_NAME_LENGTH, &block_name_len);

        std::vector<GLchar> block_name(block_name_len);
        glGetActiveUniformBlockName(m_program, block_id, block_name.size(), nullptr, block_name.data());

        GLuint binding = NextBinding();
        glUniformBlockBinding(m_program, block_id, binding);

        std::string name = block_name.data();
        m_cbv_bindings[name] = binding;
    }

    {
        GLint num_uniform = 0;
        glGetProgramInterfaceiv(m_program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &num_uniform);

        std::vector<GLenum> properties = { GL_NAME_LENGTH, GL_LOCATION };
        std::vector<GLint> values(properties.size());

        for (GLint i = 0; i < num_uniform; ++i)
        {
            glGetProgramResourceiv(m_program, GL_UNIFORM, i, properties.size(),
                properties.data(), values.size(), nullptr, values.data());

            GLint& name_len = values[0];
            GLint& location = values[1];

            std::vector<GLchar> nameData(name_len);
            glGetProgramResourceName(m_program, GL_UNIFORM, i, nameData.size(), nullptr, &nameData[0]);
            std::string name((char*)&nameData[0], nameData.size() - 1);

            auto loc = name.find("SPIRV_Cross_Combined");
            if (loc == -1)
                continue;

            m_texture_loc[name] = { m_texture_loc.size(), location };
        }
    }
}

void GLProgramApi::ParseSSBO()
{
    GLint num_ssbo = 0;
    glGetProgramInterfaceiv(m_program, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &num_ssbo);

    std::vector<GLenum> properties = { GL_NAME_LENGTH };
    std::vector<GLint> values(properties.size());

    for (GLint i = 0; i < num_ssbo; ++i)
    {
        glGetProgramResourceiv(m_program, GL_SHADER_STORAGE_BLOCK, i, properties.size(),
            properties.data(), values.size(), nullptr, values.data());

        GLint& name_len = values[0];

        std::vector<GLchar> nameData(name_len);
        glGetProgramResourceName(m_program, GL_SHADER_STORAGE_BLOCK, i, nameData.size(), nullptr, &nameData[0]);
        std::string name((char*)&nameData[0], nameData.size() - 1);

        GLuint ssbo_binding_point_index = NextSSBOBinding();
        glShaderStorageBlockBinding(m_program, i, ssbo_binding_point_index);
        m_cbv_bindings[name] = ssbo_binding_point_index;

        GLint index = glGetProgramResourceIndex(m_program, GL_SHADER_STORAGE_BLOCK, name.c_str());
    }
}

void GLProgramApi::ParseShaders()
{
    GLint numActiveAttribs = 0;
    glGetProgramInterfaceiv(m_program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &numActiveAttribs);

    std::vector<GLenum> properties = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION };
    std::vector<GLint> values(properties.size());

    for (GLint i = 0; i < numActiveAttribs; ++i)
    {
        glGetProgramResourceiv(m_program, GL_PROGRAM_INPUT, i, properties.size(),
            properties.data(), values.size(), nullptr, values.data());

        GLint& name_len = values[0];
        GLint& type = values[1];
        GLint& attrib = values[2];

        std::vector<GLchar> nameData(name_len);
        glGetProgramResourceName(m_program, GL_PROGRAM_INPUT, i, nameData.size(), nullptr, &nameData[0]);
        std::string name((char*)&nameData[0], nameData.size() - 1);

        glEnableVertexArrayAttrib(m_vao, attrib);
        GLenum vertex_type = AttribComponentType(type);
        if (vertex_type == GL_UNSIGNED_INT)
        {
            glVertexArrayAttribIFormat(m_vao, attrib, AttribColumnCount(type), vertex_type, 0);
        }
        else
        {
            glVertexArrayAttribFormat(m_vao, attrib, AttribColumnCount(type), vertex_type, GL_FALSE, 0);
        }
        glVertexArrayAttribBinding(m_vao, attrib, attrib);
    }
}

void GLProgramApi::ParseShadersOuput()
{
    GLint numActiveAttribs = 0;
    glGetProgramInterfaceiv(m_program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &numActiveAttribs);

    std::vector<GLenum> properties = { GL_NAME_LENGTH };
    std::vector<GLint> values(properties.size());

    for (GLint i = 0; i < numActiveAttribs; ++i)
    {
        m_attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
        glGetProgramResourceiv(m_program, GL_PROGRAM_OUTPUT, i, properties.size(),
            properties.data(), values.size(), nullptr, values.data());

        GLint& name_len = values[0];

        std::vector<GLchar> nameData(name_len);
        glGetProgramResourceName(m_program, GL_PROGRAM_OUTPUT, i, nameData.size(), nullptr, &nameData[0]);
        std::string name((char*)&nameData[0], nameData.size() - 1);
    }
}

void GLProgramApi::OnPresent()
{
}

ShaderBlob GLProgramApi::GetBlobByType(ShaderType type) const
{
    return {};
}

void GLProgramApi::OnAttachSRV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires)
{
    if (!ires)
        return;

    GLResource& res = static_cast<GLResource&>(*ires);
    if (res.res_type == GLResource::Type::kBuffer)
    {
        auto it = m_cbv_bindings.find(name);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, it->second, res.buffer);
    }
    else
    {
        std::pair<GLint, GLint> loc = { -1, -1 };
        for (auto &x : m_samplers)
        {
            std::string glsl_name = "SPIRV_Cross_Combined" + name + x;
            auto it = m_texture_loc.find(glsl_name);
            if (it != m_texture_loc.end())
            {
                loc = it->second;
                break;
            }
        }
        if (loc.second != -1)
        {
            glProgramUniform1i(m_program, loc.second, loc.first);
            glBindTextureUnit(loc.first, res.texture);
        }
        else
        {
            int b = 0;
        }
    }
}

void GLProgramApi::OnAttachUAV(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr & ires)
{
    if (!ires)
        return;

    GLResource& res = static_cast<GLResource&>(*ires);
    if (res.res_type == GLResource::Type::kBuffer)
    {
        auto it = m_cbv_bindings.find(name);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, it->second, res.buffer);
    }
    else
    {
        GLint loc = glGetProgramResourceLocation(m_program, GL_UNIFORM, name.c_str());
        glProgramUniform1i(m_program, loc, slot);
        glBindImageTexture(slot, res.texture, 0, false, 1, GL_READ_ONLY, res.image.int_format);
    }
}

void GLProgramApi::OnAttachCBV(ShaderType type, uint32_t slot, const Resource::Ptr & ires)
{
}

void GLProgramApi::OnAttachSampler(ShaderType type, const std::string& name, uint32_t slot, const Resource::Ptr& ires)
{
    m_samplers.insert(name);
}

void GLProgramApi::OnAttachRTV(uint32_t slot, const Resource::Ptr & ires)
{
    GLResource& res = static_cast<GLResource&>(*ires);
    glNamedFramebufferTexture(m_framebuffer, GL_COLOR_ATTACHMENT0 + slot, res.texture, 0);
}

void GLProgramApi::OnAttachDSV(const Resource::Ptr & ires)
{
    GLResource& res = static_cast<GLResource&>(*ires);
    glNamedFramebufferTexture(m_framebuffer, GL_DEPTH_ATTACHMENT, res.texture, 0);
}

void GLProgramApi::ClearRenderTarget(uint32_t slot, const FLOAT ColorRGBA[4])
{
    glClearNamedFramebufferfv(m_framebuffer, GL_COLOR, slot, ColorRGBA);
}

void GLProgramApi::ClearDepthStencil(UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
    glClearNamedFramebufferfv(m_framebuffer, GL_DEPTH, 0, &Depth);
}

void GLProgramApi::SetRasterizeState(const RasterizerDesc & desc)
{
}

void GLProgramApi::SetBlendState(const BlendDesc & desc)
{
}

void GLProgramApi::SetDepthStencilState(const DepthStencilDesc& desc)
{
}

void GLProgramApi::AttachCBuffer(ShaderType type, const std::string & name, uint32_t slot, BufferLayout & buffer_layout)
{
    CommonProgramApi::AttachCBuffer(type, name, slot, buffer_layout);

    m_cbv_buffer.emplace(std::piecewise_construct,
        std::forward_as_tuple(type, slot),
        std::forward_as_tuple(m_context.CreateBuffer(BindFlag::kCbv, static_cast<uint32_t>(buffer_layout.GetBuffer().size()), 0)));
}
