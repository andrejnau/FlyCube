#include "scene.h"

tScenes::tScenes()
    : axis_x(1.0f, 0.0f, 0.0f)
    , axis_y(0.0f, 1.0f, 0.0f)
    , axis_z(0.0f, 0.0f, 1.0f)
    , modelOfFile("model/sponza/sponza.obj")
    , modelOfFileSphere("model/sphere.obj")
    , model_scale(0.01f)
{
}

inline bool tScenes::init()
{
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.2f, 0.4f, 1.0f);

    c_textureID = loadCubemap();

    depthTexture = TextureCreateDepth(depth_size, depth_size);
    depthFBO = FBODepthCreate(depthTexture);

    return true;
}

inline GLuint tScenes::FBODepthCreate(GLuint _Texture_depth)
{
    GLuint FBO = 0;
    GLenum fboStatus;
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _Texture_depth, 0);
    if ((fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE)
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

inline GLuint tScenes::TextureCreateDepth(GLsizei width, GLsizei height)
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

inline GLuint tScenes::loadCubemap()
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

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
        unsigned char * image = SOIL_load_image(textures_faces[i].c_str(), &width, &height, &channels, SOIL_LOAD_RGB);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, (GLint)GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        SOIL_free_image_data(image);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, (GLint)GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, (GLint)GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, (GLint)GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, (GLint)GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, (GLint)GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return textureID;
}

inline void tScenes::draw()
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
    lightPos = glm::vec3(light_r * cos(angle), 25.0f, light_r * sin(angle));

    GLfloat near_plane = 0.1f, far_plane = 128.0f, size = 30.0f;
    lightProjection = glm::ortho(-size, size, -size, size, near_plane, far_plane);
    lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
    lightSpaceMatrix = lightProjection * lightView;

    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);
    glViewport(0, 0, depth_size, depth_size);
    draw_obj_depth();
    glViewport(0, 0, m_width, m_height);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_BACK);
    draw_obj();

    draw_shadow();

    draw_cubemap();
}

inline void tScenes::draw_obj()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderLight.program);
    glm::mat4 projection, view, model;
    m_camera.GetMatrix(projection, view, model);

    model = glm::scale(glm::vec3(model_scale)) * model;

    glUniformMatrix4fv(shaderLight.loc.model, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(shaderLight.loc.view, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(shaderLight.loc.projection, 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3fv(shaderLight.loc.viewPos, 1, glm::value_ptr(m_camera.GetCameraPos()));
    glUniform3fv(shaderLight.loc.lightPos, 1, glm::value_ptr(lightPos));

    glm::mat4 biasMatrix(
        0.5, 0.0, 0.0, 0.0,
        0.0, 0.5, 0.0, 0.0,
        0.0, 0.0, 0.5, 0.0,
        0.5, 0.5, 0.5, 1.0);

    glm::mat4 depthBiasMVP = biasMatrix * lightSpaceMatrix;

    glUniformMatrix4fv(shaderLight.loc.depthBiasMVP, 1, GL_FALSE, glm::value_ptr(depthBiasMVP));

    Light light;
    light.ambient = glm::vec3(0.2f);
    light.diffuse = glm::vec3(1.0f);
    light.specular = glm::vec3(0.5f);

    glUniform3fv(shaderLight.loc.light.ambient, 1, glm::value_ptr(light.ambient));
    glUniform3fv(shaderLight.loc.light.diffuse, 1, glm::value_ptr(light.diffuse));
    glUniform3fv(shaderLight.loc.light.specular, 1, glm::value_ptr(light.specular));

    for (Mesh & cur_mesh : modelOfFile.meshes)
    {
        glDisable(GL_BLEND);

        glUniform1i(glGetUniformLocation(shaderLight.program, "textures.ambient"), 0);
        glUniform1i(glGetUniformLocation(shaderLight.program, "textures.has_ambient"), 0);
        glUniform1i(glGetUniformLocation(shaderLight.program, "textures.diffuse"), 0);
        glUniform1i(glGetUniformLocation(shaderLight.program, "textures.has_diffuse"), 0);
        glUniform1i(glGetUniformLocation(shaderLight.program, "textures.specular"), 0);
        glUniform1i(glGetUniformLocation(shaderLight.program, "textures.has_specular"), 0);
        glUniform1i(glGetUniformLocation(shaderLight.program, "textures.normalMap"), 0);
        glUniform1i(glGetUniformLocation(shaderLight.program, "textures.has_normalMap"), 0);
        glUniform1i(glGetUniformLocation(shaderLight.program, "textures.alpha"), 0);
        glUniform1i(glGetUniformLocation(shaderLight.program, "textures.has_alpha"), 0);

        int depth_unit = cur_mesh.textures.size() + 1;
        glUniform1i(glGetUniformLocation(shaderLight.program, "depthTexture"), depth_unit);
        glUniform1i(glGetUniformLocation(shaderLight.program, "has_depthTexture"), 0);

        for (GLuint i = 0; i < cur_mesh.textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            if (cur_mesh.textures[i].type == aiTextureType_AMBIENT)
            {
                glUniform1i(glGetUniformLocation(shaderLight.program, "textures.ambient"), i);
                glUniform1i(glGetUniformLocation(shaderLight.program, "textures.has_ambient"), 1);
            }
            if (cur_mesh.textures[i].type == aiTextureType_DIFFUSE)
            {
                glUniform1i(glGetUniformLocation(shaderLight.program, "textures.diffuse"), i);
                glUniform1i(glGetUniformLocation(shaderLight.program, "textures.has_diffuse"), 1);
            }
            else if (cur_mesh.textures[i].type == aiTextureType_SPECULAR)
            {
                glUniform1i(glGetUniformLocation(shaderLight.program, "textures.specular"), i);
                glUniform1i(glGetUniformLocation(shaderLight.program, "textures.has_specular"), 1);
            }
            else if (cur_mesh.textures[i].type == aiTextureType_HEIGHT)
            {
                glUniform1i(glGetUniformLocation(shaderLight.program, "textures.normalMap"), i);
                glUniform1i(glGetUniformLocation(shaderLight.program, "textures.has_normalMap"), 1);
            }
            else if (cur_mesh.textures[i].type == aiTextureType_OPACITY)
            {
                glUniform1i(glGetUniformLocation(shaderLight.program, "textures.alpha"), i);
                glUniform1i(glGetUniformLocation(shaderLight.program, "textures.has_alpha"), 1);

                glEnable(GL_BLEND);
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }

            glBindTexture(GL_TEXTURE_2D, cur_mesh.textures[i].id);
        }

        glActiveTexture(GL_TEXTURE0 + depth_unit);
        glBindTexture(GL_TEXTURE_2D, depthTexture);

        glUniform1i(glGetUniformLocation(shaderLight.program, "depthTexture"), depth_unit);
        glUniform1i(glGetUniformLocation(shaderLight.program, "has_depthTexture"), 1);

        glActiveTexture(GL_TEXTURE0);

        Material material;
        material.ambient = cur_mesh.material.amb;
        material.diffuse = cur_mesh.material.dif;
        material.specular = cur_mesh.material.spec;
        material.shininess = cur_mesh.material.shininess;

        glUniform3fv(shaderLight.loc.material.ambient, 1, glm::value_ptr(material.ambient));
        glUniform3fv(shaderLight.loc.material.diffuse, 1, glm::value_ptr(material.diffuse));
        glUniform3fv(shaderLight.loc.material.specular, 1, glm::value_ptr(material.specular));
        glUniform1f(shaderLight.loc.material.shininess, material.shininess);

        cur_mesh.bindMesh();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cur_mesh.EBO);
        glDrawElements(GL_TRIANGLES, (GLsizei)cur_mesh.indices.size(), GL_UNSIGNED_INT, 0);
        cur_mesh.unbindMesh();
    }

    glUseProgram(shaderSimpleColor.program);
    model = glm::translate(glm::mat4(1.0f), lightPos) * glm::scale(glm::mat4(1.0f), glm::vec3(0.001f));
    glUniformMatrix4fv(shaderSimpleColor.loc.u_MVP, 1, GL_FALSE, glm::value_ptr(projection * view * model));
    glUniform3f(shaderSimpleColor.loc.objectColor, 1.0f, 1.0f, 1.0);

    for (Mesh & cur_mesh : modelOfFileSphere.meshes)
    {
        cur_mesh.bindMesh();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cur_mesh.EBO);
        glDrawElements(GL_TRIANGLES, (GLsizei)cur_mesh.indices.size(), GL_UNSIGNED_INT, 0);
        cur_mesh.unbindMesh();
    }
}

inline void tScenes::draw_obj_depth()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderDepth.program);
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(model_scale));
    glm::mat4 Matrix = lightSpaceMatrix * model;
    glUniformMatrix4fv(shaderDepth.loc.u_m4MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

    for (Mesh & cur_mesh : modelOfFile.meshes)
    {
        cur_mesh.bindMesh();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cur_mesh.EBO);
        glDrawElements(GL_TRIANGLES, (GLsizei)cur_mesh.indices.size(), GL_UNSIGNED_INT, 0);
        cur_mesh.unbindMesh();
    }
}

inline void tScenes::draw_shadow()
{
    glUseProgram(shaderShadowView.program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUniform1i(glGetUniformLocation(shaderShadowView.program, "sampler"), 0);

    glm::mat4 Matrix(1.0f);

    glUniformMatrix4fv(shaderShadowView.loc.u_m4MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

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

    glBindVertexArray(0);
    glDisableVertexAttribArray(POS_ATTRIB);
    glDisableVertexAttribArray(NORMAL_ATTRIB);
    glDisableVertexAttribArray(TEXTURE_ATTRIB);
    glDisableVertexAttribArray(TANGENT_ATTRIB);
    glDisableVertexAttribArray(BITANGENT_ATTRIB);
    glDisableVertexAttribArray(POS_ATTRIB);
    glDisableVertexAttribArray(TEXTURE_ATTRIB);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glEnableVertexAttribArray(POS_ATTRIB);
    glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, plane_vertices);

    glEnableVertexAttribArray(TEXTURE_ATTRIB);
    glVertexAttribPointer(TEXTURE_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, plane_texcoords);

    glDrawElements(GL_TRIANGLES, sizeof(plane_elements) / sizeof(*plane_elements), GL_UNSIGNED_INT, plane_elements);
}

inline void tScenes::draw_cubemap()
{
    glUseProgram(shaderSimpleCubeMap.program);
    glBindTexture(GL_TEXTURE_CUBE_MAP, c_textureID);

    GLint OldCullFaceMode;
    glGetIntegerv(GL_CULL_FACE_MODE, &OldCullFaceMode);
    GLint OldDepthFuncMode;
    glGetIntegerv(GL_DEPTH_FUNC, &OldDepthFuncMode);

    glCullFace(GL_FRONT);
    glDepthFunc(GL_LEQUAL);

    glm::mat4 projection, view, model;
    m_camera.GetMatrix(projection, view, model);

    model = glm::translate(glm::mat4(1.0f), m_camera.GetCameraPos()) * glm::scale(glm::mat4(1.0f), glm::vec3(0.25f)) * model;

    glm::mat4 Matrix = projection * view * model;
    glUniformMatrix4fv(shaderSimpleCubeMap.loc.u_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

    for (Mesh & cur_mesh : modelOfFileSphere.meshes)
    {
        cur_mesh.bindMesh();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cur_mesh.EBO);
        glDrawElements(GL_TRIANGLES, (GLsizei)cur_mesh.indices.size(), GL_UNSIGNED_INT, 0);
        cur_mesh.unbindMesh();
    }

    glCullFace(OldCullFaceMode);
    glDepthFunc(OldDepthFuncMode);
}

inline void tScenes::resize(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
    m_width = width;
    m_height = height;
    m_camera.SetViewport(x, y, width, height);
}

inline void tScenes::destroy()
{
}

Camera& tScenes::getCamera()
{
    return m_camera;
}
