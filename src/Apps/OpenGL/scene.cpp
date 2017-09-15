#include "scene.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

tScenes::tScenes()
    : load_func_(ogl_LoadFunctions())
    , width_(0)
    , height_(0)
    , shader_geometry_pass_("shaders/OpenGL/GeometryPass.vs", "shaders/OpenGL/GeometryPass.fs")
    , shader_light_pass_("shaders/OpenGL/LightPass.vs", "shaders/OpenGL/LightPass.fs")
    , model_("model/sponza/sponza.obj")
    , model_square("model/square.obj", ~aiProcess_FlipUVs)
{}

std::unique_ptr<ISample> tScenes::Create()
{
    return std::make_unique<tScenes>();
}

void APIENTRY gl_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *msg, const void *data)
{
    std::cout << "debug call: " << msg << std::endl;
}

inline void tScenes::OnInit(int width, int height)
{
    width_ = width;
    height_ = height;

    InitState();
    InitCamera();
    InitGBuffer();

    for (GLMesh & cur_mesh : model_.meshes)
    {
        cur_mesh.setupMesh();
    }

    for (GLMesh & cur_mesh : model_square.meshes)
    {
        cur_mesh.setupMesh();
    }

    if (glDebugMessageCallback)
        glDebugMessageCallback(gl_callback, nullptr);
}

void tScenes::OnUpdate()
{
    UpdateCameraMovement();
    UpdateAngle();

    auto& constant_buffer_geometry_pass = shader_geometry_pass_.GetVSCBuffer("ConstantBuffer");
    auto& cb_light_pass_ = shader_light_pass_.GetPSCBuffer("ConstantBuffer");
    UpdateCBuffers(constant_buffer_geometry_pass, cb_light_pass_);
    cb_light_pass_.Update();
    constant_buffer_geometry_pass.Update();
}

inline void tScenes::OnRender()
{
    glBindFramebuffer(GL_FRAMEBUFFER, ds_fbo_);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GeometryPass();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    LightPass();
}

inline void tScenes::OnDestroy()
{}

inline void tScenes::OnSizeChanged(int width, int height)
{
    width_ = width;
    height_ = height;
    glViewport(0, 0, width_, height_);
    camera_.SetViewport(width_, height_);
}

glm::mat4 tScenes::StoreMatrix(const glm::mat4& m)
{
    return m;
}

void BindingTextures(GLProgram& shader, const std::vector<std::string>& tex)
{
    for (int i = 0; i < tex.size(); ++i)
    {
        shader.Uniform(tex[i]) = i;
    }
}

inline void tScenes::GeometryPass()
{
    glUseProgram(shader_geometry_pass_.GetProgram());

    shader_geometry_pass_.GetVSCBuffer("ConstantBuffer").SetOnPipeline();
    shader_geometry_pass_.GetPSCBuffer("Light").SetOnPipeline();
    shader_geometry_pass_.GetPSCBuffer("Material").SetOnPipeline();
    shader_geometry_pass_.GetPSCBuffer("TexturesEnables").SetOnPipeline();

    auto& light_buffer_geometry_pass = shader_geometry_pass_.GetPSCBuffer("Light");
    SetLight(light_buffer_geometry_pass);
    light_buffer_geometry_pass.Update();

    BindingTextures(
        shader_geometry_pass_,
        {
            "normalMap",
            "alphaMap",
            "ambientMap",
            "diffuseMap",
            "specularMap",
            "glossMap"
        }
    );

    for (GLMesh & cur_mesh : model_.meshes)
    {
        auto& textures_enables = shader_geometry_pass_.GetPSCBuffer("TexturesEnables");
        std::vector<int> use_textures = MapTextures(cur_mesh, textures_enables);
        textures_enables.Update();

        for (int i = 0; i < use_textures.size(); ++i)
        {
            int tex_id = use_textures[i];
            int loc_tex = 0;
            if (tex_id >= 0)
                loc_tex = cur_mesh.textures_id[tex_id];

            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, loc_tex);
        }

        auto& material_buffer_geometry_pass = shader_geometry_pass_.GetPSCBuffer("Material");
        SetMaterial(material_buffer_geometry_pass, cur_mesh);
        material_buffer_geometry_pass.Update();

        cur_mesh.drawMesh();
    }
}

class BindTextureGuard
{
public:
    BindTextureGuard(const std::vector<GLuint>& tex_id)
        : m_count(tex_id.size())
    {
        for (int i = 0; i < tex_id.size(); ++i)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, tex_id[i]);
        }
        glActiveTexture(GL_TEXTURE0);
    }

    ~BindTextureGuard()
    {
        for (int i = 0; i < m_count; ++i)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glActiveTexture(GL_TEXTURE0);
    }
private:
    size_t m_count;
};

inline void tScenes::LightPass()
{
    glUseProgram(shader_light_pass_.GetProgram());

    shader_light_pass_.GetPSCBuffer("ConstantBuffer").SetOnPipeline();

    BindingTextures(
        shader_light_pass_,
        {
            "gPosition",
            "gNormal",
            "gAmbient",
            "gDiffuse",
            "gSpecular"
        }
    );

    BindTextureGuard texture_guard({
        g_position_,
        g_normal_,
        g_ambient_,
        g_diffuse_,
        g_specular_
    });

    for (GLMesh & cur_mesh : model_square.meshes)
    {
        cur_mesh.drawMesh();
    }
}

void tScenes::InitState()
{
    glViewport(0, 0, width_, height_);
    glClearColor(0.0f, 0.2f, 0.4f, 1.0f);
    glEnable(GL_DEPTH_TEST);
}

void tScenes::InitCamera()
{
    camera_.SetViewport(width_, height_);
}

void tScenes::InitGBuffer()
{
    glGenFramebuffers(1, &ds_fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, ds_fbo_);

    // - Position buffer
    glGenTextures(1, &g_position_);
    glBindTexture(GL_TEXTURE_2D, g_position_);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, width_, height_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat borderColor[] = { 1.0, 1.0, 1.0, -1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_position_, 0);

    // - Normal buffer
    glGenTextures(1, &g_normal_);
    glBindTexture(GL_TEXTURE_2D, g_normal_);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, width_, height_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g_normal_, 0);

    // - Ambient buffer
    glGenTextures(1, &g_ambient_);
    glBindTexture(GL_TEXTURE_2D, g_ambient_);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, width_, height_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, g_ambient_, 0);

    // - Diffuse buffer
    glGenTextures(1, &g_diffuse_);
    glBindTexture(GL_TEXTURE_2D, g_diffuse_);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, width_, height_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, g_diffuse_, 0);

    // - Specular buffer
    glGenTextures(1, &g_specular_);
    glBindTexture(GL_TEXTURE_2D, g_specular_);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, width_, height_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, g_specular_, 0);

    // - Depth
    glGenTextures(1, &g_depth_texture_);
    glBindTexture(GL_TEXTURE_2D, g_depth_texture_);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, width_, height_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g_depth_texture_, 0);

    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        DBG("glCheckFramebufferStatus error 0x%X\n", fboStatus);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    GLuint attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
    glDrawBuffers(5, attachments);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
