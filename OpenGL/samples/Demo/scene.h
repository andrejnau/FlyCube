#pragma once
#define GLM_FORCE_RADIANS
#include <scenebase.h>
#include "shaders.h"
#include "geometry.h"
#include <state.h>
#include <ctime>
#include <chrono>
#include <memory>
#include <SOIL.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <simple_camera/camera.h>

class tScenes : public SceneBase, public SingleTon < tScenes >
{
public:
    tScenes()
        : axis_x(1.0f, 0.0f, 0.0f)
        , axis_y(0.0f, 1.0f, 0.0f)
        , axis_z(0.0f, 0.0f, 1.0f)
        , modelOfFile("model/sponza/sponza.obj")
        , modelOfFileSphere("model/sphere.obj")
    {
    }

    GLuint depthFBO = 0;
    GLuint depthTexture = 0;

    GLuint FBODepthCreate(GLuint _Texture_depth)
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

    GLuint TextureCreate(GLsizei width, GLsizei height)
    {
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, (GLint)GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);
        return texture;
    }

    GLuint TextureCreateDepth(GLsizei width, GLsizei height)
    {
        GLuint texture;
        // запросим у OpenGL свободный индекс текстуры
        glGenTextures(1, &texture);
        // сделаем текстуру активной
        glBindTexture(GL_TEXTURE_2D, texture);
        // установим параметры фильтрации текстуры - линейная фильтрация
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)GL_LINEAR);
        // установим параметры "оборачиваниея" текстуры - отсутствие оборачивания
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)GL_CLAMP_TO_BORDER);
        // необходимо для использования depth-текстуры как shadow map
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, (GLint)GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, (GLint)GL_LEQUAL);


        GLfloat border[4] = { 1.0, 0.0, 0.0, 0.0 };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

        // соаздем "пустую" текстуру под depth-данные
        glTexStorage2D(GL_TEXTURE_2D, 1, (GLenum)GL_DEPTH_COMPONENT24, width, height);

        glBindTexture(GL_TEXTURE_2D, 0);
        return texture;
    }

    glm::mat4 lightSpaceMatrix;
    glm::vec3 lightPos;
    glm::mat4 lightProjection;
    glm::mat4 lightView;

    virtual bool init()
    {
        checkGlError("pre");

        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.2f, 0.4f, 1.0f);

        depthTexture = TextureCreateDepth(m_width, m_height);
        depthFBO = FBODepthCreate(depthTexture);

        lightPos = glm::vec3(1.0, 25.0, 0.0f);

        GLfloat near_plane = 1.0f, far_plane = 25.0f;
        lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
        lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;

        return true;
    }

    virtual void draw()
    {
        auto & state = CurState<bool>::Instance().state;
        if (state["warframe"])
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now(), end = std::chrono::system_clock::now();

        end = std::chrono::system_clock::now();
        int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        start = std::chrono::system_clock::now();
        float dt = std::min(0.001f, elapsed / 1500.0f);
        angle += dt;


        glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_TRUE);
        glClear(GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_FRONT);
        draw_obj_depth();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_BACK);
        draw_obj();

        draw_shadow();
    }

    void draw_obj()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderLight.program);
        glm::mat4 projection, view, model;
        m_camera.GetMatrix(projection, view, model);


        //projection = lightProjection;
        //view = lightView;
        model = glm::scale(glm::vec3(0.01)) * model;

        glUniformMatrix4fv(shaderLight.loc_model, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(shaderLight.loc_view, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(shaderLight.loc_projection, 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3fv(shaderLight.loc_viewPos, 1, glm::value_ptr(m_camera.GetCameraPos()));
        glUniform3fv(shaderLight.loc_lightPos, 1, glm::value_ptr(lightPos));

        glm::mat4 biasMatrix(
            0.5, 0.0, 0.0, 0.0,
            0.0, 0.5, 0.0, 0.0,
            0.0, 0.0, 0.5, 0.0,
            0.5, 0.5, 0.5, 1.0
            );

        glm::mat4 depthBiasMVP = biasMatrix * lightSpaceMatrix;

        glUniformMatrix4fv(shaderLight.loc_depthBiasMVP, 1, GL_FALSE, glm::value_ptr(depthBiasMVP));

        Light light;
        light.ambient = 0.5f * glm::vec3(1.0f);
        light.diffuse = 1.5f * glm::vec3(1.0f);
        light.specular = 1.0f * glm::vec3(1.0f);

        glUniform3fv(shaderLight.loc_light.ambient, 1, glm::value_ptr(light.ambient));
        glUniform3fv(shaderLight.loc_light.diffuse, 1, glm::value_ptr(light.diffuse));
        glUniform3fv(shaderLight.loc_light.specular, 1, glm::value_ptr(light.specular));

        for (Mesh & cur_mesh : modelOfFile.meshes)
        {
            glDisable(GL_BLEND);

            if (std::string(cur_mesh.material.name.C_Str()) == "16___Default")
                continue;

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

            glUniform3fv(shaderLight.loc_material.ambient, 1, glm::value_ptr(material.ambient));
            glUniform3fv(shaderLight.loc_material.diffuse, 1, glm::value_ptr(material.diffuse));
            glUniform3fv(shaderLight.loc_material.specular, 1, glm::value_ptr(material.specular));
            glUniform1f(shaderLight.loc_material.shininess, material.shininess);

            validateProgram(shaderLight.program);

            cur_mesh.bindMesh();
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cur_mesh.EBO);
            glDrawElements(GL_TRIANGLES, (GLsizei)cur_mesh.indices.size(), GL_UNSIGNED_INT, 0);
            cur_mesh.unbindMesh();
        }

        glUseProgram(shaderSimpleColor.program);
        model = glm::translate(glm::mat4(1.0f), lightPos) * glm::scale(glm::mat4(1.0f), glm::vec3(15.0f)) * model;
        glUniformMatrix4fv(shaderSimpleColor.loc_uMVP, 1, GL_FALSE, glm::value_ptr(projection * view * model));
        glUniform3f(shaderSimpleColor.loc_objectColor, 1.0f, 1.0f, 1.0);

        for (Mesh & cur_mesh : modelOfFileSphere.meshes)
        {
            cur_mesh.bindMesh();
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cur_mesh.EBO);
            glDrawElements(GL_TRIANGLES, (GLsizei)cur_mesh.indices.size(), GL_UNSIGNED_INT, 0);
            cur_mesh.unbindMesh();
        }
    }

    void draw_obj_depth()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderDepth.program);
        glm::mat4 model = glm::scale(glm::mat4(1.0), glm::vec3(0.01));
        glm::mat4 Matrix = lightSpaceMatrix * model;
        glUniformMatrix4fv(shaderDepth.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

        for (Mesh & cur_mesh : modelOfFile.meshes)
        {
            if (std::string(cur_mesh.material.name.C_Str()) == "16___Default")
                continue;

            cur_mesh.bindMesh();
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cur_mesh.EBO);
            glDrawElements(GL_TRIANGLES, (GLsizei)cur_mesh.indices.size(), GL_UNSIGNED_INT, 0);
            cur_mesh.unbindMesh();
        }
    }

    void draw_shadow()
    {
        glUseProgram(shaderShadowView.program);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glUniform1i(glGetUniformLocation(shaderShadowView.program, "sampler"), 0);

        glm::mat4 Matrix(1.0f);

        glUniformMatrix4fv(shaderShadowView.loc_MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

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
        checkGlError("glDrawElements");
    }

    virtual void resize(int x, int y, int width, int height)
    {
        glViewport(x, y, width, height);
        m_width = width;
        m_height = height;
        m_camera.SetViewport(x, y, width, height);

        if (width != m_width || height != m_height)
        {
            if (depthTexture)
                glDeleteTextures(1, &depthTexture);
            if (depthFBO)
                glDeleteFramebuffers(1, &depthFBO);
            depthTexture = TextureCreateDepth(width, height);
            depthFBO = FBODepthCreate(depthTexture);
        }
    }

    virtual void destroy()
    {
    }

    Camera & getCamera()
    {
        return m_camera;
    }
private:
    struct Material
    {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        float shininess;
    };

    struct Light
    {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
    };

private:
    float eps = 1e-3f;

    glm::vec3 axis_x;
    glm::vec3 axis_y;
    glm::vec3 axis_z;

    glm::vec3 lightPosition;

    int m_width;
    int m_height;

    float angle = 0.0f;

    Model modelOfFile;
    Model modelOfFileSphere;

    ShaderLight shaderLight;
    ShaderSimpleColor shaderSimpleColor;
    ShaderShadowView shaderShadowView;
    ShaderDepth shaderDepth;
    Camera m_camera;
};