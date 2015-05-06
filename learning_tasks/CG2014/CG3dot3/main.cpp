#include <GLFW/glfw3.h>
#include "scene.h" 
#include "state.h"
#include <memory>

std::unique_ptr<tScenes> renderer;

void draw_scene()
{
	renderer->draw();
}

void init_opengl()
{
	ogl_LoadFunctions();
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	renderer->resize(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_F && action == GLFW_PRESS)
	{
		auto & state = CurState<bool>::Instance().state;
		state["warframe"] = !state["warframe"];
	}

	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
	{
		renderer->read_size();
	}

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

	GLFWwindow* window = glfwCreateWindow(700, 700, "Simple CG3dot3", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, key_callback);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glfwMakeContextCurrent(window);

	init_opengl();

	/*printGlString("Version", GL_VERSION);
	printGlString("Vendor", GL_VENDOR);
	printGlString("Renderer", GL_RENDERER);
	printGlString("Extensions", GL_EXTENSIONS);*/

	renderer.reset(new tScenes());

	renderer->init();

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	framebuffer_size_callback(window, width, height);

	glfwWindowHint(GLFW_SAMPLES, 4);

	while (!glfwWindowShouldClose(window))
	{
		draw_scene();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	exit(EXIT_SUCCESS);
}
