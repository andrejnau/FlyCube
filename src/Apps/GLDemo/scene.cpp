#include "scene.h"
#include <GLFW/glfw3.h>

tScenes::tScenes()
    : load_func_(ogl_LoadFunctions())
    , width_(0)
    , height_(0)
    , shader_geometry_pass_("shaders/Demo/GeometryPass.vs", "shaders/Demo/GeometryPass.fs")
    , shader_light_pass_("shaders/Demo/LightPass.vs", "shaders/Demo/LightPass.fs")
    , shader_ssao_pass_("shaders/Demo/SSAOPass.vs", "shaders/Demo/SSAOPass.fs")
    , shader_ssao_blur_pass_("shaders/Demo/SSAOPass.vs", "shaders/Demo/SSAOBlurPass.fs")
    , shader_simple_color_("shaders/Demo/ShaderSimpleColor.vs", "shaders/Demo/ShaderSimpleColor.fs")
    , shader_simple_cube_map_("shaders/Demo/ShaderSimpleCubeMap.vs", "shaders/Demo/ShaderSimpleCubeMap.fs")
    , shader_shadow_view_("shaders/Demo/ShaderShadowView.vs", "shaders/Demo/ShaderShadowView.fs")
    , shader_depth_("shaders/Demo/ShaderDepth.vs", "shaders/Demo/ShaderDepth.fs")
    , model_("model/sponza/sponza.obj")
    , model_sphere_("model/sphere.obj")
{}

inline GLfloat lerp(GLfloat a, GLfloat b, GLfloat f)
{
    return a + f * (b - a);
}

inline void tScenes::OnInit(int width, int height)
{
    width_ = width;
    height_ = height;

    if (glDebugMessageCallback)
        glDebugMessageCallback(tScenes::gl_callback, nullptr);

    auto& state = CurState<bool>::Instance().state;
    state["occlusion"] = true;
    state["shadow"] = true;

    InitState();
    InitCamera();
    InitCubemap();
    InitShadow();
    InitGBuffer();
    InitSSAO();
    InitMesh();

    for (GLMesh & cur_mesh : model_.meshes)
    {
        cur_mesh.setupMesh();
    }

    for (GLMesh & cur_mesh : model_sphere_.meshes)
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

    GLfloat near_plane = 0.1f, far_plane = 128.0f, size = 30.0f;
    light_projection_ = glm::ortho(-size, size, -size, size, near_plane, far_plane);
    light_view_ = glm::lookAt(light_pos_, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
    light_matrix_ = light_projection_ * light_view_;
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

    if (state["shadow"])
    {
        glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo_);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glClear(GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_FRONT);
        glViewport(0, 0, depth_size_, depth_size_);
        ShadowPass();
        glViewport(0, 0, width_, height_);
    }

    if (state["occlusion"])
    {
        glBindFramebuffer(GL_FRAMEBUFFER, ssao_fbo_);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_BACK);
        SSAOPass();
        glBindFramebuffer(GL_FRAMEBUFFER, ssao_blur_fbo_);
        SSAOBlurPass();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_BACK);
    LightPass();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, ds_fbo_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    RenderLightSource();
    //RenderShadowTexture();
    RenderCubemap();
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

    shader_geometry_pass_.Uniform("model") = model;
    shader_geometry_pass_.Uniform("view") = view;
    shader_geometry_pass_.Uniform("projection") = projection;

    Light light;
    light.ambient = glm::vec3(0.2f);
    light.diffuse = glm::vec3(1.0f);
    light.specular = glm::vec3(0.5f);

    shader_geometry_pass_.Uniform("light.ambient") = light.ambient;
    shader_geometry_pass_.Uniform("light.diffuse") = light.diffuse;
    shader_geometry_pass_.Uniform("light.specular") = light.specular;

    for (GLMesh & cur_mesh : model_.meshes)
    {
        shader_geometry_pass_.Uniform("textures.ambient") = 0;
        shader_geometry_pass_.Uniform("textures.has_ambient") = 0;
        shader_geometry_pass_.Uniform("textures.diffuse") = 0;
        shader_geometry_pass_.Uniform("textures.has_diffuse") = 0;
        shader_geometry_pass_.Uniform("textures.specular") = 0;
        shader_geometry_pass_.Uniform("textures.has_specular") = 0;
        shader_geometry_pass_.Uniform("textures.normalMap") = 0;
        shader_geometry_pass_.Uniform("textures.has_normalMap") = 0;
        shader_geometry_pass_.Uniform("textures.alpha") = 0;
        shader_geometry_pass_.Uniform("textures.has_alpha") = 0;

        for (GLint i = 0; i < (GLint)cur_mesh.textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            if (cur_mesh.textures[i].type == aiTextureType_AMBIENT)
            {
                shader_geometry_pass_.Uniform("textures.ambient") = i;
                shader_geometry_pass_.Uniform("textures.has_ambient") = 1;
            }
            if (cur_mesh.textures[i].type == aiTextureType_DIFFUSE)
            {
                shader_geometry_pass_.Uniform("textures.diffuse") = i;
                shader_geometry_pass_.Uniform("textures.has_diffuse") = 1;
            }
            else if (cur_mesh.textures[i].type == aiTextureType_SPECULAR)
            {
                shader_geometry_pass_.Uniform("textures.specular") = i;
                shader_geometry_pass_.Uniform("textures.has_specular") = 1;
            }
            else if (cur_mesh.textures[i].type == aiTextureType_HEIGHT)
            {
                shader_geometry_pass_.Uniform("textures.normalMap") = i;
                auto & state = CurState<bool>::Instance().state;
                if (!state["disable_norm"])
                    shader_geometry_pass_.Uniform("textures.has_normalMap") = 1;
            }
            else if (cur_mesh.textures[i].type == aiTextureType_OPACITY)
            {
                shader_geometry_pass_.Uniform("textures.alpha") = i;
                shader_geometry_pass_.Uniform("textures.has_alpha") = 1;
            }

            glBindTexture(GL_TEXTURE_2D, cur_mesh.textures_id[i]);
        }

        glActiveTexture(GL_TEXTURE0);

        Material material;
        material.ambient = cur_mesh.material.amb;
        material.diffuse = cur_mesh.material.dif;
        material.specular = cur_mesh.material.spec;
        material.shininess = cur_mesh.material.shininess;

        shader_geometry_pass_.Uniform("material.ambient") = material.ambient;
        shader_geometry_pass_.Uniform("material.diffuse") = material.diffuse;
        shader_geometry_pass_.Uniform("material.specular") = material.specular;
        shader_geometry_pass_.Uniform("material.shininess") = material.shininess;

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

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, g_ssao_blur_);
    shader_light_pass_.Uniform("gSSAO") = 5;

    auto & state = CurState<bool>::Instance().state;
    if (state["occlusion"])
    {
        shader_light_pass_.Uniform("has_SSAO") = 1;
        if (state["occlusion_only"])
            shader_light_pass_.Uniform("occlusion_only") = 1;
        else
            shader_light_pass_.Uniform("occlusion_only") = 0;
    }
    else
    {
        shader_light_pass_.Uniform("has_SSAO") = 0;
        shader_light_pass_.Uniform("occlusion_only") = 0;
    }

    glm::vec3 cameraPosView = glm::vec3(camera_.GetViewMatrix() * glm::vec4(camera_.GetCameraPos(), 1.0));
    glm::vec3 lightPosView = glm::vec3(camera_.GetViewMatrix() * glm::vec4(light_pos_, 1.0));
    shader_light_pass_.Uniform("viewPos") = cameraPosView;
    shader_light_pass_.Uniform("lightPos") = lightPosView;

    glm::mat4 biasMatrix(
        0.5, 0.0, 0.0, 0.0,
        0.0, 0.5, 0.0, 0.0,
        0.0, 0.0, 0.5, 0.0,
        0.5, 0.5, 0.5, 1.0);

    glm::mat4 depthBiasMVP = biasMatrix * light_matrix_ * glm::inverse(camera_.GetViewMatrix());

    shader_light_pass_.Uniform("depthBiasMVP") = depthBiasMVP;

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, shadow_texture_);
    shader_light_pass_.Uniform("depthTexture") = 6;

    if (state["shadow"])
    {
        shader_light_pass_.Uniform("has_depthTexture") = 1;
    }
    else
    {
        shader_light_pass_.Uniform("has_depthTexture") = 0;
    }

    mesh_square_.drawMesh();
}

void tScenes::RenderLightSource()
{
    glUseProgram(shader_simple_color_.GetProgram());
    glm::mat4 projection, view, model;
    camera_.GetMatrix(projection, view, model);

    model = glm::translate(glm::mat4(1.0f), light_pos_) * glm::scale(glm::mat4(1.0f), glm::vec3(0.001f));
    shader_simple_color_.Uniform("u_MVP") = projection * view * model;
    shader_simple_color_.Uniform("objectColor") = glm::vec3(1.0f, 1.0f, 1.0);

    for (GLMesh & cur_mesh : model_sphere_.meshes)
    {
        cur_mesh.drawMesh();
    }
}

inline void tScenes::ShadowPass()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader_depth_.GetProgram());
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(model_scale_));
    glm::mat4 Matrix = light_matrix_ * model;
    shader_depth_.Uniform("u_m4MVP") = Matrix;

    for (GLMesh & cur_mesh : model_.meshes)
    {
        cur_mesh.drawMesh();
    }
}

void tScenes::SSAOPass()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shader_ssao_pass_.GetProgram());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_position_);
    shader_ssao_pass_.Uniform("gPosition") = 0;

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_normal_);
    shader_ssao_pass_.Uniform("gNormal") = 1;

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noise_texture_);
    shader_ssao_pass_.Uniform("noiseTexture") = 2;

    shader_ssao_pass_.Uniform("samples") = ssao_kernel_;

    glm::mat4 projection = camera_.GetProjectionMatrix();
    shader_ssao_pass_.Uniform("projection") = projection;

    mesh_square_.drawMesh();
}

void tScenes::SSAOBlurPass()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shader_ssao_blur_pass_.GetProgram());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_ssao_);
    shader_ssao_blur_pass_.Uniform("ssaoInput") = 0;

    mesh_square_.drawMesh();
}

inline void tScenes::RenderShadowTexture()
{
    glDisable(GL_DEPTH_TEST);

    glUseProgram(shader_shadow_view_.GetProgram());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadow_texture_);
    shader_shadow_view_.Uniform("sampler") = 0;

    glm::mat4 Matrix(1.0f);

    shader_shadow_view_.Uniform("u_m4MVP") = Matrix;

    mesh_square_shadow_view_.drawMesh();

    glEnable(GL_DEPTH_TEST);
}

inline void tScenes::RenderCubemap()
{
    glUseProgram(shader_simple_cube_map_.GetProgram());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture_);
    shader_shadow_view_.Uniform("sampler") = 0;

    GLint old_cull_face_mode;
    glGetIntegerv(GL_CULL_FACE_MODE, &old_cull_face_mode);
    GLint old_depth_func_mode;
    glGetIntegerv(GL_DEPTH_FUNC, &old_depth_func_mode);

    glCullFace(GL_FRONT);
    glDepthFunc(GL_LEQUAL);

    glm::mat4 projection, view, model;
    camera_.GetMatrix(projection, view, model);

    model = glm::translate(glm::mat4(1.0f), camera_.GetCameraPos()) * glm::scale(glm::mat4(1.0f), glm::vec3(0.25f)) * model;

    glm::mat4 Matrix = projection * view * model;
    shader_simple_cube_map_.Uniform("u_MVP") = Matrix;

    for (GLMesh & cur_mesh : model_sphere_.meshes)
    {
        cur_mesh.drawMesh();
    }

    glCullFace(old_cull_face_mode);
    glDepthFunc(old_depth_func_mode);
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

void tScenes::InitSSAO()
{
    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    int kernel_size = 64;
    for (int i = 0; i < kernel_size; ++i)
    {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / float(kernel_size);

        // Scale samples s.t. they're more aligned to center of kernel
        scale = lerp(0.15f, 1.0f, scale * scale);
        sample *= scale;
        ssao_kernel_.push_back(sample);
    }

    glGenFramebuffers(1, &ssao_fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, ssao_fbo_);

    glGenTextures(1, &g_ssao_);
    glBindTexture(GL_TEXTURE_2D, g_ssao_);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, width_, height_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_ssao_, 0);

    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        DBG("glCheckFramebufferStatus error 0x%X\n", fboStatus);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, attachments);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::vector<glm::vec3> ssaoNoise;
    for (GLuint i = 0; i < 16; i++)
    {
        glm::vec3 noise(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, 0.0f);
        ssaoNoise.push_back(noise);
    }

    glGenTextures(1, &noise_texture_);
    glBindTexture(GL_TEXTURE_2D, noise_texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


    glGenFramebuffers(1, &ssao_blur_fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, ssao_blur_fbo_);

    glGenTextures(1, &g_ssao_blur_);
    glBindTexture(GL_TEXTURE_2D, g_ssao_blur_);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, width_, height_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_ssao_blur_, 0);

    fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        DBG("glCheckFramebufferStatus error 0x%X\n", fboStatus);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void tScenes::InitShadow()
{
    shadow_texture_ = CreateShadowTexture(depth_size_, depth_size_);
    shadow_fbo_ = CreateShadowFBO(shadow_texture_);
}

void tScenes::InitCubemap()
{
    cubemap_texture_ = LoadCubemap();
}

void tScenes::InitMesh()
{
    std::vector<glm::vec3> position = {
        glm::vec3(-1.0, 1.0, 0.0),
        glm::vec3(1.0, 1.0, 0.0),
        glm::vec3(1.0, -1.0, 0.0),
        glm::vec3(-1.0, -1.0, 0.0)
    };

    std::vector<glm::vec3> position_shadow_view = {
        glm::vec3(-1.0, 1.0, 0.0),
        glm::vec3(-0.5, 1.0, 0.0),
        glm::vec3(-0.5, 0.5, 0.0),
        glm::vec3(-1.0, 0.5, 0.0)
    };

    std::vector<glm::vec2> texCoords = {
        glm::vec2(0.0, 1.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(0.0, 0.0)
    };

    std::vector<GLuint> indices = {
        0, 1, 2,
        2, 3, 0
    };

    mesh_square_.vertices.resize(position.size());
    mesh_square_shadow_view_.vertices.resize(position.size());
    for (size_t i = 0; i < mesh_square_.vertices.size(); ++i)
    {
        mesh_square_.vertices[i].position = position[i];
        mesh_square_shadow_view_.vertices[i].position = position_shadow_view[i];

        mesh_square_.vertices[i].texCoords = texCoords[i];
        mesh_square_shadow_view_.vertices[i].texCoords = texCoords[i];
    }

    mesh_square_.indices = indices;
    mesh_square_shadow_view_.indices = indices;

    mesh_square_.setupMesh();
    mesh_square_shadow_view_.setupMesh();
}

inline GLuint tScenes::CreateShadowFBO(GLuint shadow_texture)
{
    GLuint FBO = 0;
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_texture, 0);
    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        DBG("glCheckFramebufferStatus error 0x%X\n", fboStatus);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return 0;
    }
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return FBO;
}

inline GLuint tScenes::CreateShadowTexture(GLsizei width, GLsizei height)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)GL_CLAMP_TO_BORDER);
    GLfloat border[4] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, (GLint)GL_COMPARE_REF_TO_TEXTURE);
    glTexStorage2D(GL_TEXTURE_2D, 1, (GLenum)GL_DEPTH_COMPONENT24, width, height);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

inline GLuint tScenes::LoadCubemap()
{
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

    std::vector <std::string> textures_faces = {
        GetAssetFullPath("cubemap/skycloud/txStormydays_rt.bmp"),
        GetAssetFullPath("cubemap/skycloud/txStormydays_lf.bmp"),
        GetAssetFullPath("cubemap/skycloud/txStormydays_up.bmp"),
        GetAssetFullPath("cubemap/skycloud/txStormydays_dn.bmp"),
        GetAssetFullPath("cubemap/skycloud/txStormydays_bk.bmp"),
        GetAssetFullPath("cubemap/skycloud/txStormydays_ft.bmp")
    };

    for (GLuint i = 0; i < textures_faces.size(); i++)
    {
        int width, height, channels;
        unsigned char *image = SOIL_load_image(textures_faces[i].c_str(), &width, &height, &channels, SOIL_LOAD_RGBA);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, (GLint)GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        SOIL_free_image_data(image);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, (GLint)GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, (GLint)GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, (GLint)GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, (GLint)GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, (GLint)GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return texture_id;
}
