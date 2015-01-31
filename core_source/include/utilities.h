#pragma once

#include <platform.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <glm/glm.hpp>

static bool checkGlError(const char* funcName)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		ALOGE("GL error after %s(): 0x%08x\n", funcName, err);
		return true;
	}
	return false;
}

static GLuint createShader(GLenum shaderType, const char* src)
{
	GLuint shader = glCreateShader(shaderType);
	if (!shader)
	{
		checkGlError("glCreateShader");
		return 0;
	}
	glShaderSource(shader, 1, &src, NULL);

	GLint compiled = 0;
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);


	if (compiled == 0)
	{
		GLint infoLogLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);
		if (infoLogLen > 0)
		{
			GLchar* infoLog = new GLchar[infoLogLen];
			if (infoLog)
			{
				glGetShaderInfoLog(shader, infoLogLen, NULL, infoLog);
				ALOGE("Could not compile %s shader:\n%s\n",
					shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment",
					infoLog);
				delete[] infoLog;
			}
		}
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

static GLuint createProgram(const char* vtxSrc, const char* fragSrc)
{
	GLuint vtxShader = 0;
	GLuint fragShader = 0;
	GLuint program = 0;
	GLint linked = 0;

	vtxShader = createShader(GL_VERTEX_SHADER, vtxSrc);
	if (!vtxShader)
		goto exit;

	fragShader = createShader(GL_FRAGMENT_SHADER, fragSrc);
	if (!fragShader)
		goto exit;

	program = glCreateProgram();
	if (!program)
	{
		checkGlError("glCreateProgram");
		goto exit;
	}
	glAttachShader(program, vtxShader);
	glAttachShader(program, fragShader);

	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (linked == 0)
	{
		ALOGE("Could not link program");
		GLint infoLogLen = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLen);
		if (infoLogLen)
		{
			GLchar* infoLog = new GLchar[infoLogLen];
			if (infoLog)
			{
				glGetProgramInfoLog(program, infoLogLen, NULL, infoLog);
				ALOGE("Could not link program:\n%s\n", infoLog);
				delete[] infoLog;
			}
		}
		glDeleteProgram(program);
		program = 0;
	}

exit:
	glDeleteShader(vtxShader);
	glDeleteShader(fragShader);
	return program;
}

static void printGlString(const char* name, GLenum s)
{
	const char* v = (const char*)glGetString(s);
	ALOGV("GL %s: %s\n", name, v);
}

#if defined(ANDROID) || defined(__ANDROID__)

class AAssetFile
{
public:
	static AAssetFile & getInstance()
	{
		static AAssetFile instance;
		return instance;
	}

	std::vector<char> read(const std::string &path)
	{
		std::vector<char> ret;
		AAsset* asset = AAssetManager_open(mgr, path.c_str(), AASSET_MODE_RANDOM);
		if (asset != 0)
		{
			int len = AAsset_getLength(asset);
			ret.resize(len);
			AAsset_read(asset, &ret[0], len);
			AAsset_close(asset);
		};
		return ret;
	}

	void setManager(AAssetManager* pAssetMgr)
	{
		mgr = pAssetMgr;
	}
private:
	AAssetManager* mgr;
};

#endif

static std::stringstream getStream(const std::string &path)
{
#if defined(ANDROID) || defined(__ANDROID__)
	std::vector<char> v(AAssetFile::getInstance().read(path));
	std::string str(v.begin(), v.end());
	return std::stringstream(str);
#else

	std::ifstream is(path, std::ios::in);
	std::stringstream buf;
	buf << is.rdbuf();
	return buf;
#endif
}

static bool loadOBJ(const std::string &path,
	std::vector<glm::vec3> & vertices,
	std::vector<glm::vec2> & uvs,
	std::vector<glm::vec3> & normals)
{
	std::ios_base::sync_with_stdio(false);

	std::stringstream buf, buf_split;
	buf = std::move(getStream(path));

	std::vector<GLuint> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	std::string type, tmp;

	while (buf >> type)
	{
		if (type == "v")
		{
			glm::vec3 one_vertex;

			buf >> tmp;
			one_vertex.x = std::stof(tmp);

			buf >> tmp;
			one_vertex.y = std::stof(tmp);

			buf >> tmp;
			one_vertex.z = std::stof(tmp);

			temp_vertices.push_back(one_vertex);
		}
		else if (type == "vt")
		{
			glm::vec2 one_uv;

			buf >> tmp;
			one_uv.x = std::stof(tmp);

			buf >> tmp;
			one_uv.y = std::stof(tmp);

			temp_uvs.push_back(one_uv);
		}
		else if (type == "vn")
		{
			glm::vec3 one_normal;

			buf >> tmp;
			one_normal.x = std::stof(tmp);

			buf >> tmp;
			one_normal.y = std::stof(tmp);

			buf >> tmp;
			one_normal.z = std::stof(tmp);

			temp_normals.push_back(one_normal);
		}
		else if (type == "f")
		{
			std::string line;
			buf.get();
			std::getline(buf, line);
			std::replace(line.begin(), line.end(), '/', ' ');
			buf_split.clear();
			buf_split << line;

			for (int i = 0; i < 3; ++i)
			{
				GLuint vertexIndex, uvIndex, normalIndex;

				buf_split >> tmp;
				vertexIndex = std::stoi(tmp);

				buf_split >> tmp;
				uvIndex = std::stoi(tmp);

				buf_split >> tmp;
				normalIndex = std::stoi(tmp);

				vertexIndices.push_back(vertexIndex);
				uvIndices.push_back(uvIndex);
				normalIndices.push_back(normalIndex);
			}
		}
	}
	vertices.resize(vertexIndices.size());
	uvs.resize(vertexIndices.size());
	normals.resize(vertexIndices.size());
	for (size_t i = 0; i < vertexIndices.size(); ++i)
	{
		vertices[i] = temp_vertices[vertexIndices[i] - 1];
		uvs[i] = temp_uvs[uvIndices[i] - 1];
		normals[i] = temp_normals[normalIndices[i] - 1];
	}

	return true;
}