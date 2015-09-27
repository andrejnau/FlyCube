#pragma once
#include <platform.h>
#include <utilities.h>
#include <scenebase.h>
#include <algorithm>
#include <math.h>
#include <string>
#include <vector>
#include <list>
#include <ctime>
#include <chrono>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QImage>
#include <QGLWidget>

#define STR(s) #s
#define STRV(s) STR(s)

#define POS_ATTRIB 0
#define NORMAL_ATTRIB 1
#define TEXTURE_ATTRIB 2


static const char VERTEX_SHADERPQ[] =
"#version 300 es\n"
"precision mediump float;\n"
"layout(location = " STRV(POS_ATTRIB) ") in vec2 pos;\n"
"layout(location = " STRV(TEXTURE_ATTRIB) ") in vec2 tex;\n"
"out vec2 texCoord;\n"
"uniform mat4 u_m4MVP;\n"
"void main() {\n"
"    texCoord = tex;\n"
"    gl_Position = u_m4MVP * vec4(pos, 0.0, 1.0);\n"
"}\n";

static const char FRAGMENT_SHADERPQ[] =
"#version 300 es\n"
"precision mediump float;\n"
"uniform sampler2D sampler2d;\n"
"out vec4 outColor;\n"
"in vec2 texCoord;\n"
"void main() {\n"
"    outColor.rgb = texture(sampler2d, texCoord).rgb;\n"
"    outColor.a = 1.0;\n"
"}\n";

static const char VERTEX_SHADERPQF[] =
"#version 300 es\n"
"precision mediump float;\n"
"layout(location = " STRV(POS_ATTRIB) ") in vec2 pos;\n"
"layout(location = " STRV(TEXTURE_ATTRIB) ") in vec3 tex;\n"
"out vec3 texCoord;\n"
"uniform mat4 u_m4MVP;\n"
"void main() {\n"
"    texCoord = tex;\n"
"    gl_Position = u_m4MVP * vec4(pos, 0.0, 1.0);\n"
"}\n";

static const char FRAGMENT_SHADERPQF[] =
"#version 300 es\n"
"precision mediump float;\n"
"out vec4 outColor;\n"
"in vec3 texCoord;\n"
"void main() {\n"
"    outColor.rgb = texCoord;\n"
"    outColor.a = 1.0;\n"
"}\n";

class PrintImage : public SceneBase
{
public:
	PrintImage()
	{
	}

	bool init()
	{
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		mProgram = createProgram(VERTEX_SHADERPQF, FRAGMENT_SHADERPQF);
		if (!mProgram)
			return false;

		loc_u_m4MVP = glGetUniformLocation(mProgram, "u_m4MVP");
		return true;
	}

	void setImage(QImage img, std::vector<glm::vec2> &p)
	{
		point = p;
		if (!point.empty())
			point.pop_back();

		/*point.pop_back();
		glm::vec2 a = point[0] - point[1];
		glm::vec2 b = point[2] - point[1];
	    point.push_back(point[1] + (a + b));*/
		m_img = img;

	}

	bool inPolugon(glm::vec2 val)
	{
		std::vector<float> sing;
		for (int i = 0; i < (int)point.size(); ++i)
		{
			glm::vec2 next1 = point[(i + 0) % point.size()];
			glm::vec2 next2 = point[(i + 1) % point.size()];

			glm::vec2 vec1 = next2 - next1;
			glm::vec2 vec2 = val - next1;

			sing.push_back(vec1.x * vec2.y - vec1.y * vec2.x);
		}
		bool f1 = true, f2 = true;
		for (int i = 0; i < (int)sing.size(); ++i)
		{
			if (!sing[i])
				continue;
			f1 = f1 & (sing[i] < 0);
			f2 = f2 & (sing[i] > 0);
		}

		return (f1 ^ f2);

	}

	void init_all()
	{
		img_point.clear();
		img_color.clear();

		float XPixelSize = 2.0f * 1.0f / m_width;
		float YPixelSize = 2.0f * 1.0f / m_height;

		float ratio = (GLfloat)m_height / (GLfloat)m_width;

		float x_b = -1.0f;
		float x_e = 1.0f;
		float y_b = -1.0f;
		float y_e = 1.0f;

		if (m_width >= m_height)
		{
			x_b /= ratio;
			x_e /= ratio;
		}
		else
		{
			y_b *= ratio;
			y_e *= ratio;
		}
		
		std::vector<glm::vec2> q;
		q.push_back(glm::vec2(0.0f, 0.0f));
		q.push_back(glm::vec2(m_img.width() - 1, 0.0f));
		q.push_back(glm::vec2(m_img.width() - 1, m_img.height() - 1));
		q.push_back(glm::vec2(0.0f, m_img.height()-1));

		std::vector<glm::vec2> s = point;
		glm::mat2 tmp;
		tmp[0][0] = q[1].x - q[0].x;
		tmp[0][1] = q[1].y - q[0].y;
		tmp[1][0] = q[2].x - q[1].x;
		tmp[1][1] = q[2].y - q[1].y;
		
		glm::mat2 A; 
		A[0][0] = s[1].x - s[0].x;
		A[0][1] = s[1].y - s[0].y;
		A[1][0] = s[2].x - s[1].x;
		A[1][1] = s[2].y - s[1].y;
		
		std::swap(A[1][0], A[0][1]);
		std::swap(tmp[1][0], tmp[0][1]);

		A = glm::inverse(A) * tmp;	

		glm::vec2 B = q[0] - (s[0] * A);		

		for (float i = x_b; i <= x_e; i += XPixelSize)
		{
			for (float j = y_b; j <= y_e; j += YPixelSize)
			{
				glm::vec2 cur = glm::vec2(i, j);
				if (inPolugon(cur))
				{
					img_point.push_back(cur);
					glm::vec2 tex = cur * A + B;
					QPoint p(tex.x, tex.y);
					QRgb value = m_img.pixel(p);
					int r, g, b, a;
					QColor(value).getRgb(&r, &g, &b, &a);
					glm::vec3 color = glm::vec3(r, g, b);
					color /= 255.0f;

					img_color.push_back(color);
				}
			}
		}
	}

	void draw()
	{
		glClearColor(0.9411f, 0.9411f, 0.9411f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(mProgram);
		glUniformMatrix4fv(loc_u_m4MVP, 1, GL_FALSE, glm::value_ptr(Matrix));

		int cnt = img_point.size();
		glVertexAttribPointer(POS_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, img_point.data());
		glEnableVertexAttribArray(POS_ATTRIB);
		glVertexAttribPointer(TEXTURE_ATTRIB, 3, GL_FLOAT, GL_FALSE, 0, img_color.data());
		glEnableVertexAttribArray(TEXTURE_ATTRIB);

		glDrawArrays(GL_POINTS, 0, cnt);
	}

	glm::vec2 display_to_gl(glm::vec2 val)
	{
		GLfloat ratio = (GLfloat)m_height / (GLfloat)m_width;
		double XPixelSize = 2.0f * (1.0f * val.x / m_width);
		double YPixelSize = 2.0f * (1.0f * val.y / m_height);
		double Xposition = XPixelSize - 1.0f;
		double Yposition = (2.0f - YPixelSize) - 1.0f;
		if (m_width >= m_height)
			Xposition /= ratio;
		else
			Yposition *= ratio;
		return glm::vec2(float(Xposition), float(Yposition));
	}

	void setImage2(QImage img, std::vector<glm::vec2> &p)
	{
		glGenVertexArrays(1, &vaoObjectPoint);
		glGenBuffers(1, &vboVertexPoint);
		glGenBuffers(1, &vboVertexPointTex);

		point = p;
		if (!point.empty())
			point.pop_back();
		m_img = img;
		QImage GL_formatted_image;
		GL_formatted_image = QGLWidget::convertToGLFormat(img);

		glGenTextures(1, &texrure);
		glBindTexture(GL_TEXTURE_2D, texrure);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GL_formatted_image.width(), GL_formatted_image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, GL_formatted_image.bits());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindVertexArray(vaoObjectPoint);
		glBindBuffer(GL_ARRAY_BUFFER, vboVertexPoint);
		glBufferData(GL_ARRAY_BUFFER, 2 * point.size() * sizeof(glm::vec2), point.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(POS_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(POS_ATTRIB);

		static GLfloat tex[] = { -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f };

		glBindBuffer(GL_ARRAY_BUFFER, vboVertexPointTex);
		glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec2), tex, GL_STATIC_DRAW);
		glVertexAttribPointer(TEXTURE_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(TEXTURE_ATTRIB);

		glBindVertexArray(0);
	}

	void draw2()
	{
		static GLuint elem[] = { 0, 1, 2, 2, 3, 0 };

		glClearColor(0.9411f, 0.9411f, 0.9411f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(mProgram);
		glBindTexture(GL_TEXTURE_2D, texrure);
		glUniformMatrix4fv(loc_u_m4MVP, 1, GL_FALSE, glm::value_ptr(Matrix));
		glBindVertexArray(vaoObjectPoint);
		glDrawElements(GL_TRIANGLES, sizeof(elem) / sizeof(*elem), GL_UNSIGNED_INT, &elem);
		glBindVertexArray(0);
	}

	void resize(int x, int y, int width, int height)
	{
		m_x = x;
		m_y = y;
		m_width = width;
		m_height = height;

		glViewport(m_x, m_y, m_width, m_height);
		GLfloat ratio = (GLfloat)m_height / (GLfloat)m_width;
		if (m_width >= m_height)
			Matrix = glm::ortho(-1.0f / ratio, 1.0f / ratio, -1.0f, 1.0f, -10.0f, 1.0f);
		else
			Matrix = glm::ortho(-1.0f, 1.0f, -1.0f * ratio, 1.0f*ratio, -10.0f, 1.0f);

		init_all();
	}

	void destroy()
	{
	}
private:
	QImage m_img;
	GLuint texrure;

	std::vector<glm::vec2> img_point;
	std::vector<glm::vec3> img_color;

	GLuint mProgram;

	GLuint vaoObjectPoint;
	GLuint vboVertexPoint, vboVertexPointTex;

	glm::mat4 Matrix;

	std::vector<glm::vec2> point;
	std::vector<glm::vec2> point_tex;

	int m_width;
	int m_height;
	int m_x;
	int m_y;

	GLint loc_u_m4MVP;
};
