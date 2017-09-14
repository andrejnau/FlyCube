#include "scene.h"
#include <GLFW/glfw3.h>

tScenes::tScenes()
    : load_func_(ogl_LoadFunctions())
    , width_(0)
    , height_(0)
    , shader_geometry_pass_("shaders/OpenGL/GeometryPass.vs", "shaders/OpenGL/GeometryPass.fs")
    , shader_light_pass_("shaders/OpenGL/LightPass.vs", "shaders/OpenGL/LightPass.fs")
    , model_("model/sponza/sponza.obj")
    , model_square("model/square.obj", ~aiProcess_FlipUVs)
{}

inline void tScenes::OnInit(int width, int height)
{
    width_ = width;
    height_ = height;

    glDebugMessageCallback(tScenes::gl_callback, nullptr);

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
}

void tScenes::OnUpdate()
{
    UpdateCameraMovement();    

    auto & state = CurState<bool>::Instance().state;

    static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    static std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
    static float angle = 0.0;

    end = std::chrono::system_clock::now();
    int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    start = std::chrono::system_clock::now();

    if (!state["pause"])
        angle += elapsed / 2e6f;

    float light_r = 2.5;
    light_pos_ = glm::vec3(light_r * cos(angle), 25.0f, light_r * sin(angle));
}

inline void tScenes::OnRender()
{
    auto & state = CurState<bool>::Instance().state;

    glBindFramebuffer(GL_FRAMEBUFFER, ds_fbo_);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_BACK);

    if (state["warframe"])
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    GeometryPass();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_BACK);
    LightPass();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, ds_fbo_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

void tScenes::OnKey(int key, int action)
{
    if (action == GLFW_PRESS)
        keys_[key] = true;
    else if (action == GLFW_RELEASE)
        keys_[key] = false;

    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        auto & state = CurState<bool>::Instance().state;
        state["warframe"] = !state["warframe"];
    }

    if (key == GLFW_KEY_N && action == GLFW_PRESS)
    {
        auto & state = CurState<bool>::Instance().state;
        state["disable_norm"] = !state["disable_norm"];
    }

    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        auto & state = CurState<bool>::Instance().state;
        state["occlusion"] = !state["occlusion"];
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        auto & state = CurState<bool>::Instance().state;
        state["pause"] = !state["pause"];
    }

    if (key == GLFW_KEY_L && action == GLFW_PRESS)
    {
        auto & state = CurState<bool>::Instance().state;
        state["shadow"] = !state["shadow"];
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        auto & state = CurState<bool>::Instance().state;
        state["occlusion_only"] = !state["occlusion_only"];
    }
}

void tScenes::OnMouse(bool first_event, double xpos, double ypos)
{
    if (first_event)
    {
        last_x_ = xpos;
        last_y_ = ypos;
    }

    double xoffset = xpos - last_x_;
    double yoffset = last_y_ - ypos;  // Reversed since y-coordinates go from bottom to left

    last_x_ = xpos;
    last_y_ = ypos;

    camera_.ProcessMouseMovement((float)xoffset, (float)yoffset);
}

inline void tScenes::GeometryPass()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shader_geometry_pass_.GetProgram());
    glm::mat4 projection, view, model;
    camera_.GetMatrix(projection, view, model);

    model = glm::scale(glm::vec3(model_scale_)) * model;

    auto& constant_buffer_geometry_pass = shader_geometry_pass_.GetVSCBuffer("ConstantBuffer");
    constant_buffer_geometry_pass.Uniform("model") = model;
    constant_buffer_geometry_pass.Uniform("view") = view;
    constant_buffer_geometry_pass.Uniform("projection") = projection;
    constant_buffer_geometry_pass.Update();
    constant_buffer_geometry_pass.SetOnPipeline();

    Light light;
    light.ambient = glm::vec3(0.2f);
    light.diffuse = glm::vec3(1.0f);
    light.specular = glm::vec3(0.5f);

    auto& light_buffer_geometry_pass = shader_geometry_pass_.GetPSCBuffer("Light");
    light_buffer_geometry_pass.Uniform("light_ambient") = light.ambient;
    light_buffer_geometry_pass.Uniform("light_diffuse") = light.diffuse;
    light_buffer_geometry_pass.Uniform("light_specular") = light.specular;
    light_buffer_geometry_pass.Update();
    light_buffer_geometry_pass.SetOnPipeline();

    for (GLMesh & cur_mesh : model_.meshes)
    {
        auto& textures_buffer_geometry_pass = shader_geometry_pass_.GetPSCBuffer("TexturesEnables");
        textures_buffer_geometry_pass.Uniform("has_diffuseMap") = 0;
        textures_buffer_geometry_pass.Uniform("has_normalMap") = 0;
        textures_buffer_geometry_pass.Uniform("has_specularMap") = 0;
        textures_buffer_geometry_pass.Uniform("has_glossMap") = 0;
        textures_buffer_geometry_pass.Uniform("has_ambientMap") = 0;
        textures_buffer_geometry_pass.Uniform("has_alphaMap") = 0;

        for (GLint i = 0; i < (GLint)cur_mesh.textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            if (cur_mesh.textures[i].type == aiTextureType_AMBIENT)
            {
                shader_geometry_pass_.Uniform("ambientMap") = i;
                textures_buffer_geometry_pass.Uniform("has_ambientMap") = 1;
            }
            if (cur_mesh.textures[i].type == aiTextureType_DIFFUSE)
            {
                shader_geometry_pass_.Uniform("diffuseMap") = i;
                textures_buffer_geometry_pass.Uniform("has_diffuseMap") = 1;
            }
            else if (cur_mesh.textures[i].type == aiTextureType_SPECULAR)
            {
                shader_geometry_pass_.Uniform("specularMap") = i;
                textures_buffer_geometry_pass.Uniform("has_specularMap") = 1;
            }
            else if (cur_mesh.textures[i].type == aiTextureType_HEIGHT)
            {
                shader_geometry_pass_.Uniform("normalMap") = i;
                auto & state = CurState<bool>::Instance().state;
                if (!state["disable_norm"])
                    textures_buffer_geometry_pass.Uniform("has_normalMap") = 1;
            }
            else if (cur_mesh.textures[i].type == aiTextureType_OPACITY)
            {
                shader_geometry_pass_.Uniform("alphaMap") = i;
                textures_buffer_geometry_pass.Uniform("has_alphaMap") = 1;
            }

            glBindTexture(GL_TEXTURE_2D, cur_mesh.textures_id[i]);
        }

        textures_buffer_geometry_pass.Update();
        textures_buffer_geometry_pass.SetOnPipeline();

        glActiveTexture(GL_TEXTURE0);

        auto& material_buffer_geometry_pass = shader_geometry_pass_.GetPSCBuffer("Material");
        Material material;
        material.ambient = cur_mesh.material.amb;
        material.diffuse = cur_mesh.material.dif;
        material.specular = cur_mesh.material.spec;
        material.shininess = cur_mesh.material.shininess;

        material_buffer_geometry_pass.Uniform("material_ambient") = material.ambient;
        material_buffer_geometry_pass.Uniform("material_diffuse") = material.diffuse;
        material_buffer_geometry_pass.Uniform("material_specular") = material.specular;
        material_buffer_geometry_pass.Uniform("material_shininess") = material.shininess;
        material_buffer_geometry_pass.Update();
        material_buffer_geometry_pass.SetOnPipeline();

        cur_mesh.drawMesh();
    }
}

inline void tScenes::LightPass()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shader_light_pass_.GetProgram());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_position_);
    shader_light_pass_.Uniform("gPosition") = 0;

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_normal_);
    shader_light_pass_.Uniform("gNormal") = 1;

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, g_ambient_);
    shader_light_pass_.Uniform("gAmbient") = 2;

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, g_diffuse_);
    shader_light_pass_.Uniform("gDiffuse") = 3;

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, g_specular_);
    shader_light_pass_.Uniform("gSpecular") = 4;

    glm::vec3 cameraPosView = glm::vec3(camera_.GetViewMatrix() * glm::vec4(camera_.GetCameraPos(), 1.0));
    glm::vec3 lightPosView = glm::vec3(camera_.GetViewMatrix() * glm::vec4(light_pos_, 1.0));

    auto& cb_light_pass_ = shader_light_pass_.GetPSCBuffer("ConstantBuffer");
    cb_light_pass_.Uniform("viewPos") = cameraPosView;
    cb_light_pass_.Uniform("lightPos") = lightPosView;
    cb_light_pass_.Update();
    cb_light_pass_.SetOnPipeline();

    for (GLMesh & cur_mesh : model_square.meshes)
    {
        cur_mesh.drawMesh();
    }
}

void tScenes::UpdateCameraMovement()
{
    double currentFrame = glfwGetTime();
    delta_time_ = (float)(currentFrame - last_frame_);
    last_frame_ = currentFrame;

    if (keys_[GLFW_KEY_W])
        camera_.ProcessKeyboard(CameraMovement::kForward, delta_time_);
    if (keys_[GLFW_KEY_S])
        camera_.ProcessKeyboard(CameraMovement::kBackward, delta_time_);
    if (keys_[GLFW_KEY_A])
        camera_.ProcessKeyboard(CameraMovement::kLeft, delta_time_);
    if (keys_[GLFW_KEY_D])
        camera_.ProcessKeyboard(CameraMovement::kRight, delta_time_);
    if (keys_[GLFW_KEY_Q])
        camera_.ProcessKeyboard(CameraMovement::kDown, delta_time_);
    if (keys_[GLFW_KEY_E])
        camera_.ProcessKeyboard(CameraMovement::kUp, delta_time_);
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
