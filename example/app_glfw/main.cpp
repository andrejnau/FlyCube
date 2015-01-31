#include <GLFW/glfw3.h>
#include <getscene.h>

SceneBase &renderer = GetScene::get(SceneType::TestScene);

void draw_scene()
{
	renderer.draw();
}

void init_opengl(void)
{
	ogl_LoadFunctions();
	renderer.init();
}

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	renderer.resize(0, 0, width, height);
}

int main(void)
{
	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	GLFWwindow* window = glfwCreateWindow(700, 700, "Simple example", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glfwMakeContextCurrent(window);

	init_opengl();

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	framebuffer_size_callback(window, width, height);

	while (!glfwWindowShouldClose(window))
	{
		draw_scene();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	exit(EXIT_SUCCESS);
}
