#include <GLFW/glfw3.h>
#include "scene.h"
#include "state.h"
#include <memory>

std::unique_ptr<tScenes> renderer;

void draw_scene()
{
	renderer->draw();
}

void init_scene(int width, int height)
{
	glbinding::Binding::initialize();
	renderer.reset(new tScenes());
	renderer->resize(0, 0, width, height);
	renderer->init();
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	renderer->resize(0, 0, width, height);
}

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

int main(void)
{
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(700, 700, "Simple LoadTexture", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glfwMakeContextCurrent(window);

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	init_scene(width, height);

	printGlString("GL_VERSION", GL_VERSION);

	while (!glfwWindowShouldClose(window))
	{
		draw_scene();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	exit(EXIT_SUCCESS);
}
