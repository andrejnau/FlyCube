#pragma once
#include <platform.h>
#include <utilities.h>
#include <scenebase.h>
#include <math.h>
#include <string>
#include <vector>
#include <ctime>
#include <chrono>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STR(s) #s
#define STRV(s) STR(s)

#define POS_ATTRIB 0
#define NORMAL_ATTRIB 1
#define TEXTURE_ATTRIB 2

static const char VERTEX_SHADER[] =
"#version 300 es\n"
"precision mediump float;\n"
"layout(location = " STRV(POS_ATTRIB) ") in vec3 pos;\n"
"layout(location = " STRV(NORMAL_ATTRIB) ") in vec3 normal;\n"
"uniform mat4 u_m4MVP;\n"
"uniform mat4 u_m4VP;\n"
"out vec3 modelViewVertex;\n"
"out vec3 modelViewNormal;\n"
"out vec3 modelNormal;\n"
"void main() {\n"
"    gl_Position = u_m4MVP * vec4(pos, 1.0);\n"
"    modelViewVertex = vec3(u_m4MVP * vec4(pos, 1.0));\n"
"    modelViewNormal = normalize(vec3(u_m4MVP * vec4(normal, 0.0)));\n"
"    modelNormal = normalize(vec3(u_m4VP * vec4(normal, 0.0)));\n"
"}\n";

static const char FRAGMENT_SHADER[] =
"#version 300 es\n"
"precision mediump float;\n"
"uniform vec3 u_lightPosition;\n"
"uniform vec3 u_camera;\n"
"out vec4 outColor;\n"
"in vec3 modelViewVertex;\n"
"in vec3 modelViewNormal;\n"
"in vec3 modelNormal;\n"
"void main() {\n"
"    vec3 lightVector = normalize(-u_lightPosition - modelViewVertex);\n"
"    float diffuse = max(dot(modelViewNormal, lightVector), 0.1);\n"
"    float distance = length(u_lightPosition - modelViewVertex);\n"
"    diffuse = diffuse * (1.0 / (1.0 + (0.025 * distance * distance)));\n"
"    vec3 lookVector = normalize(u_camera - modelViewVertex);\n"
"    vec3 reflectvector = abs(reflect(-lightVector, modelNormal));\n"
"    float specular = pow( max(dot(lookVector, reflectvector), 0.0), 100.0);\n"
"    outColor.rgb = vec3(1.0, 0.0, 0.0) * diffuse + vec3(0.0, 0.0, 1.0) * 0.0 * specular;\n"
"    outColor.a = 1.0;\n"
"}\n";

class TestScene : public SceneBase, public SingleTon < TestScene >
{
public:
	TestScene()
		: axis_x(1.0f, 0.0f, 0.0f),
		axis_y(0.0f, 1.0f, 0.0f),
		axis_z(0.0f, 0.0f, 1.0f)
	{
	}

	bool init()
	{
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glClearColor(0.365f, 0.54f, 0.66f, 1.0f);

		mProgram = createProgram(VERTEX_SHADER, FRAGMENT_SHADER);
		if (!mProgram)
			return false;

		loc_u_m4MVP = glGetUniformLocation(mProgram, "u_m4MVP");
		loc_u_m4VP = glGetUniformLocation(mProgram, "u_m4VP");

		loc_lightPosition = glGetUniformLocation(mProgram, "u_lightPosition");
		loc_camera = glGetUniformLocation(mProgram, "u_camera");

		glGenVertexArrays(1, &vaoSuzannet);
		glBindVertexArray(vaoSuzannet);

#if defined(ANDROID) || defined(__ANDROID__)
		std::string m_path("suzanne.obj");
#else
		std::string m_path(PROJECT_RESOURCE_DIR "/obj-model/suzanne.obj");
#endif

		std::vector<glm::vec3> vertices;
		std::vector<glm::vec2> uvs;
		std::vector<glm::vec3> normals;
		bool res = loadOBJ(m_path, vertices, uvs, normals);
		MLOG(mlog::debug, "status: %d", res);
		suzanneSize = vertices.size();

		GLuint vboVertexS, vboNormalS;
		glGenBuffers(1, &vboVertexS);
		glGenBuffers(1, &vboNormalS);

		glBindBuffer(GL_ARRAY_BUFFER, vboVertexS);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(POS_ATTRIB);

		glBindBuffer(GL_ARRAY_BUFFER, vboNormalS);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(NORMAL_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(NORMAL_ATTRIB);
		glBindVertexArray(0);

		return true;
	}

	void destroy()
	{

	}

	void draw_cube()
	{
		static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now(), end = std::chrono::system_clock::now();

		end = std::chrono::system_clock::now();
		int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		start = std::chrono::system_clock::now();

		static float f = 0;
		static float fl = 0;
		fl += elapsed / 1000.0f;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(mProgram);

		glm::vec3 lightPosition(2.0f * sin(fl), 2.0f * sin(fl), 2.0f  * sin(fl));
		glm::vec3 camera(0.0f, 0.0f, 2.0f);

		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 view = glm::lookAt(
			camera,
			glm::vec3(0, 0, 0),
			glm::vec3(0, 1, 0)
			);
		glm::mat4 projection = glm::perspective(
			45.0f,
			(float)m_width / m_height,
			0.1f,
			100.0f
			);

		glm::mat4 animX = glm::rotate(glm::mat4(1.0f), 0.0f, axis_x);
		glm::mat4 animY = glm::rotate(glm::mat4(1.0f), f, axis_y);

		model = animX * animY * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.6f, 0.6f, 0.6f));

		glm::mat4 Matrix = projection * view * model;
		glm::mat4 MatrixPV = projection * view;

		glUniformMatrix4fv(loc_u_m4VP, 1, GL_FALSE, glm::value_ptr(MatrixPV));
		glUniformMatrix4fv(loc_u_m4MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

		glUniform3fv(loc_lightPosition, 1, glm::value_ptr(lightPosition));
		glUniform3fv(loc_camera, 1, glm::value_ptr(camera));

		glBindVertexArray(vaoSuzannet);
		glDrawArrays(GL_TRIANGLES, 0, suzanneSize);
		glBindVertexArray(0);
	}

	void draw()
	{
		static GLfloat bkColor[4];
		glGetFloatv(GL_COLOR_CLEAR_VALUE, bkColor);
	    //glClearColor(0.365f, 0.54f, 0.66f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		draw_cube();
	}

	void resize(int x, int y, int width, int height)
	{
		glViewport(x, y, width, height);
		m_width = width;
		m_height = height;
	}
private:
	glm::vec3 axis_x;
	glm::vec3 axis_y;
	glm::vec3 axis_z;

	int m_width;
	int m_height;

	GLuint mProgram;

	GLuint vaoObject;
	GLuint vaoSuzannet;

	size_t suzanneSize;

	GLint loc_u_m4MVP;
	GLint loc_u_m4VP;
	GLint loc_lightPosition;
	GLint loc_camera;
};
