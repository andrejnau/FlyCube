#include <GLFW/glfw3.h>
#include "scene.h" 
#include "state.h"
#include <memory>

std::unique_ptr<tScenes> renderer;

void draw_scene()
{
	renderer->draw();
}


void init_opengl(void)
{
	renderer.reset(new tScenes());
	renderer->init();
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

	float eps = 0.1f;

	switch (key) {
	case GLFW_KEY_W:
		renderer->getCamera().Move(FORWARD);
		break;
	case GLFW_KEY_A:
		renderer->getCamera().Move(LEFT);
		break;
	case GLFW_KEY_S:
		renderer->getCamera().Move(BACK);
		break;
	case GLFW_KEY_D:
		renderer->getCamera().Move(RIGHT);
		break;
	case GLFW_KEY_Q:
		renderer->getCamera().Move(DOWN);
		break;
	case GLFW_KEY_E:
		renderer->getCamera().Move(UP);
		break;
	case GLFW_KEY_L:
		renderer->getCamera().angle_y += eps;
		break;
	case GLFW_KEY_J:
		renderer->getCamera().angle_y -= eps;
		break;
	case GLFW_KEY_I:
		renderer->getCamera().angle_x += eps;
		break;
	case GLFW_KEY_K:
		renderer->getCamera().angle_x -= eps;
		break;
	case GLFW_KEY_O:
		renderer->getCamera().angle_z += eps;
		break;
	case GLFW_KEY_U:
		renderer->getCamera().angle_z -= eps;
		break;
	case GLFW_KEY_ESCAPE:
		exit(0);
		break;
	default:
		break;
	}
	renderer->getCamera().Update();
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

	GLFWwindow* window = glfwCreateWindow(700, 700, "Simple CGPhysical", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, key_callback);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glfwMakeContextCurrent(window);

	ogl_LoadFunctions();

	printGlString("Version", GL_VERSION);
	printGlString("Vendor", GL_VENDOR);
	printGlString("Renderer", GL_RENDERER);
	printGlString("Extensions", GL_EXTENSIONS);

	init_opengl();

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
