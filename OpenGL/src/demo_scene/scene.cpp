#include "scene.h"

tScenes::tScenes(int width, int height)
    : width_(width)
    , height_(height)
    , axis_x_(1.0f, 0.0f, 0.0f)
    , axis_y_(0.0f, 1.0f, 0.0f)
    , axis_z_(0.0f, 0.0f, 1.0f)
    , model_("model/sponza/sponza.obj")
    , model_sphere_("model/sphere.obj")
    , model_scale_(0.01f)
{}

inline void tScenes::OnInit()
{
    camera_.SetViewport(width_, height_);

    glViewport(0, 0, width_, height_);
    glClearColor(0.0f, 0.2f, 0.4f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    cubemap_texture_ = LoadCubemap();
    shadow_texture_ = CreateShadowTexture(depth_size_, depth_size_);
    shadow_fbo_ = CreateShadowFBO(shadow_texture_);

    InitGBuffer();
}

void tScenes::OnUpdate()
{
    auto & state = CurState<bool>::Instance().state;
    if (state["warframe"])
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
    static std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
    static float angle = 0.0;

    end = std::chrono::system_clock::now();
    int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    start = std::chrono::system_clock::now();

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
    glBindFramebuffer(GL_FRAMEBUFFER, g_buffer_);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_BACK);
    GeometryPass();

    glBindFramebuffer(GL_FRAMEBUFFER, shadow_fbo_);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);
    glViewport(0, 0, depth_size_, depth_size_);
    ShadowPass();
    glViewport(0, 0, width_, height_);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_BACK);
    LightPass();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, g_buffer_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    RenderLightSource();
    RenderShadowTexture();
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

inline void tScenes::GeometryPass()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shader_geometry_pass_.program);
    glm::mat4 projection, view, model;
    camera_.GetMatrix(projection, view, model);

    model = glm::scale(glm::vec3(model_scale_)) * model;

    glUniformMatrix4fv(shader_geometry_pass_.loc.model, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(shader_geometry_pass_.loc.view, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(shader_geometry_pass_.loc.projection, 1, GL_FALSE, glm::value_ptr(projection));

    Light light;
    light.ambient = glm::vec3(0.2f);
    light.diffuse = glm::vec3(1.0f);
    light.specular = glm::vec3(0.5f);

    glUniform3fv(shader_geometry_pass_.loc.light.ambient, 1, glm::value_ptr(light.ambient));
    glUniform3fv(shader_geometry_pass_.loc.light.diffuse, 1, glm::value_ptr(light.diffuse));
    glUniform3fv(shader_geometry_pass_.loc.light.specular, 1, glm::value_ptr(light.specular));

    for (Mesh & cur_mesh : model_.meshes)
    {
        glUniform1i(shader_geometry_pass_.loc.textures.ambient, 0);
        glUniform1i(shader_geometry_pass_.loc.textures.has_ambient, 0);
        glUniform1i(shader_geometry_pass_.loc.textures.diffuse, 0);
        glUniform1i(shader_geometry_pass_.loc.textures.has_diffuse, 0);
        glUniform1i(shader_geometry_pass_.loc.textures.specular, 0);
        glUniform1i(shader_geometry_pass_.loc.textures.has_specular, 0);
        glUniform1i(shader_geometry_pass_.loc.textures.normalMap, 0);
        glUniform1i(shader_geometry_pass_.loc.textures.has_normalMap, 0);
        glUniform1i(shader_geometry_pass_.loc.textures.alpha, 0);
        glUniform1i(shader_geometry_pass_.loc.textures.has_alpha, 0);

        for (GLuint i = 0; i < cur_mesh.textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            if (cur_mesh.textures[i].type == aiTextureType_AMBIENT)
            {
                glUniform1i(shader_geometry_pass_.loc.textures.ambient, i);
                glUniform1i(shader_geometry_pass_.loc.textures.has_ambient, 1);
            }
            if (cur_mesh.textures[i].type == aiTextureType_DIFFUSE)
            {
                glUniform1i(shader_geometry_pass_.loc.textures.diffuse, i);
                glUniform1i(shader_geometry_pass_.loc.textures.has_diffuse, 1);
            }
            else if (cur_mesh.textures[i].type == aiTextureType_SPECULAR)
            {
                glUniform1i(shader_geometry_pass_.loc.textures.specular, i);
                glUniform1i(shader_geometry_pass_.loc.textures.has_specular, 1);
            }
            else if (cur_mesh.textures[i].type == aiTextureType_HEIGHT)
            {
                glUniform1i(shader_geometry_pass_.loc.textures.normalMap, i);
                glUniform1i(shader_geometry_pass_.loc.textures.has_normalMap, 1);
            }
            else if (cur_mesh.textures[i].type == aiTextureType_OPACITY)
            {
                glUniform1i(shader_geometry_pass_.loc.textures.alpha, i);
                glUniform1i(shader_geometry_pass_.loc.textures.has_alpha, 1);
            }

            glBindTexture(GL_TEXTURE_2D, cur_mesh.textures[i].id);
        }

        glActiveTexture(GL_TEXTURE0);

        Material material;
        material.ambient = cur_mesh.material.amb;
        material.diffuse = cur_mesh.material.dif;
        material.specular = cur_mesh.material.spec;
        material.shininess = cur_mesh.material.shininess;

        glUniform3fv(shader_geometry_pass_.loc.material.ambient, 1, glm::value_ptr(material.ambient));
        glUniform3fv(shader_geometry_pass_.loc.material.diffuse, 1, glm::value_ptr(material.diffuse));
        glUniform3fv(shader_geometry_pass_.loc.material.specular, 1, glm::value_ptr(material.specular));
        glUniform1f(shader_geometry_pass_.loc.material.shininess, material.shininess);

        cur_mesh.bindMesh();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cur_mesh.EBO);
        glDrawElements(GL_TRIANGLES, (GLsizei)cur_mesh.indices.size(), GL_UNSIGNED_INT, 0);
        cur_mesh.unbindMesh();
    }
}

inline void tScenes::LightPass()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shader_light_pass_.program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, g_position_);
    glUniform1i(shader_light_pass_.loc.gPosition, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, g_normal_);
    glUniform1i(shader_light_pass_.loc.gNormal, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, g_ambient_);
    glUniform1i(shader_light_pass_.loc.gAmbient, 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, g_diffuse_);
    glUniform1i(shader_light_pass_.loc.gDiffuse, 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, g_specular_);
    glUniform1i(shader_light_pass_.loc.gSpecular, 4);

    glUniform3fv(shader_light_pass_.loc.viewPos, 1, glm::value_ptr(camera_.GetCameraPos()));
    glUniform3fv(shader_light_pass_.loc.lightPos, 1, glm::value_ptr(light_pos_));

    glm::mat4 biasMatrix(
        0.5, 0.0, 0.0, 0.0,
        0.0, 0.5, 0.0, 0.0,
        0.0, 0.0, 0.5, 0.0,
        0.5, 0.5, 0.5, 1.0);

    glm::mat4 depthBiasMVP = biasMatrix * light_matrix_;

    glUniformMatrix4fv(shader_light_pass_.loc.depthBiasMVP, 1, GL_FALSE, glm::value_ptr(depthBiasMVP));

    static GLfloat plane_vertices[] = {
        -1.0, 1.0, 0.0,
        1.0, 1.0, 0.0,
        1.0, -1.0, 0.0,
        -1.0, -1.0, 0.0
    };

    static GLfloat plane_texcoords[] = {
        0.0, 1.0,
        1.0, 1.0,
        1.0, 0.0,
        0.0, 0.0
    };

    static GLuint plane_elements[] = {
        0, 1, 2,
        2, 3, 0
    };

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadow_texture_);
    glUniform1i(shader_light_pass_.loc.depthTexture, 5);
    glUniform1i(shader_light_pass_.loc.has_depthTexture, 1);

    glEnableVertexAttribArray(POS_ATTRIB);
    glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, plane_vertices);

    glEnableVertexAttribArray(TEXTURE_ATTRIB);
    glVertexAttribPointer(TEXTURE_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, plane_texcoords);

    glDrawElements(GL_TRIANGLES, sizeof(plane_elements) / sizeof(*plane_elements), GL_UNSIGNED_INT, plane_elements);

    glDisableVertexAttribArray(POS_ATTRIB);
    glDisableVertexAttribArray(TEXTURE_ATTRIB);
}

void tScenes::RenderLightSource()
{
    glUseProgram(shader_simple_color_.program);
    glm::mat4 projection, view, model;
    camera_.GetMatrix(projection, view, model);

    model = glm::translate(glm::mat4(1.0f), light_pos_) * glm::scale(glm::mat4(1.0f), glm::vec3(0.001f));
    glUniformMatrix4fv(shader_simple_color_.loc.u_MVP, 1, GL_FALSE, glm::value_ptr(projection * view * model));
    glUniform3f(shader_simple_color_.loc.objectColor, 1.0f, 1.0f, 1.0);

    for (Mesh & cur_mesh : model_sphere_.meshes)
    {
        cur_mesh.bindMesh();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cur_mesh.EBO);
        glDrawElements(GL_TRIANGLES, (GLsizei)cur_mesh.indices.size(), GL_UNSIGNED_INT, 0);
        cur_mesh.unbindMesh();
    }
}

inline void tScenes::ShadowPass()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader_depth_.program);
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(model_scale_));
    glm::mat4 Matrix = light_matrix_ * model;
    glUniformMatrix4fv(shader_depth_.loc.u_m4MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

    for (Mesh & cur_mesh : model_.meshes)
    {
        cur_mesh.bindMesh();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cur_mesh.EBO);
        glDrawElements(GL_TRIANGLES, (GLsizei)cur_mesh.indices.size(), GL_UNSIGNED_INT, 0);
        cur_mesh.unbindMesh();
    }
}

inline void tScenes::RenderShadowTexture()
{
    glUseProgram(shader_shadow_view_.program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadow_texture_);
    glUniform1i(shader_shadow_view_.loc.sampler, 0);

    glm::mat4 Matrix(1.0f);

    glUniformMatrix4fv(shader_shadow_view_.loc.u_m4MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

    static GLfloat plane_vertices[] = {
        -1.0, 1.0, 0.0,
        -0.5, 1.0, 0.0,
        -0.5, 0.5, 0.0,
        -1.0, 0.5, 0.0
    };

    static GLfloat plane_texcoords[] = {
        0.0, 1.0,
        1.0, 1.0,
        1.0, 0.0,
        0.0, 0.0
    };

    static GLuint plane_elements[] = {
        0, 1, 2,
        2, 3, 0
    };

    glEnableVertexAttribArray(POS_ATTRIB);
    glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, plane_vertices);

    glEnableVertexAttribArray(TEXTURE_ATTRIB);
    glVertexAttribPointer(TEXTURE_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, plane_texcoords);

    glDrawElements(GL_TRIANGLES, sizeof(plane_elements) / sizeof(*plane_elements), GL_UNSIGNED_INT, plane_elements);
}

inline void tScenes::RenderCubemap()
{
    glUseProgram(shader_simple_cube_map_.program);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture_);

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
    glUniformMatrix4fv(shader_simple_cube_map_.loc.u_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

    for (Mesh & cur_mesh : model_sphere_.meshes)
    {
        cur_mesh.bindMesh();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cur_mesh.EBO);
        glDrawElements(GL_TRIANGLES, (GLsizei)cur_mesh.indices.size(), GL_UNSIGNED_INT, 0);
        cur_mesh.unbindMesh();
    }

    glCullFace(old_cull_face_mode);
    glDepthFunc(old_depth_func_mode);
}

void tScenes::InitGBuffer()
{
    glGenFramebuffers(1, &g_buffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, g_buffer_);
    GLuint depth_texture;

    int numSamples = 4;

    // - Position buffer
    glGenTextures(1, &g_position_);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, g_position_);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numSamples, GL_RGB32F, width_, height_, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, g_position_, 0);

    // - Normal buffer
    glGenTextures(1, &g_normal_);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, g_normal_);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numSamples, GL_RGB32F, width_, height_, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, g_normal_, 0);

    // - Ambient buffer
    glGenTextures(1, &g_ambient_);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, g_ambient_);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numSamples, GL_RGB32F, width_, height_, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D_MULTISAMPLE, g_ambient_, 0);

    // - Diffuse buffer
    glGenTextures(1, &g_diffuse_);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, g_diffuse_);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numSamples, GL_RGB32F, width_, height_, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D_MULTISAMPLE, g_diffuse_, 0);

    // - Specular buffer
    glGenTextures(1, &g_specular_);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, g_specular_);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numSamples, GL_RGBA32F, width_, height_, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D_MULTISAMPLE, g_specular_, 0);

    // - Depth
    glGenTextures(1, &depth_texture);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, depth_texture);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numSamples, GL_DEPTH_COMPONENT32, width_, height_, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)GL_CLAMP_TO_BORDER);
    GLfloat border[4] = { 1.0, 0.0, 0.0, 0.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, (GLint)GL_COMPARE_REF_TO_TEXTURE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depth_texture, 0);

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
    GLfloat border[4] = { 1.0, 0.0, 0.0, 0.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, (GLint)GL_COMPARE_REF_TO_TEXTURE);
    glTexStorage2D(GL_TEXTURE_2D, 1, (GLenum)GL_DEPTH_COMPONENT32, width, height);
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

Camera& tScenes::getCamera()
{
    return camera_;
}