#include <GLFW/glfw3.h>
#include <scene.h>
#include <state.h>
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

	float dt = 0.1f;
	switch (key)
	{
	case GLFW_KEY_W:
		renderer->getCamera().ProcessKeyboard(FORWARD, dt);
		break;
	case GLFW_KEY_A:
		renderer->getCamera().ProcessKeyboard(LEFT, dt);
		break;
	case GLFW_KEY_S:
		renderer->getCamera().ProcessKeyboard(BACKWARD, dt);
		break;
	case GLFW_KEY_D:
		renderer->getCamera().ProcessKeyboard(RIGHT, dt);
		break;
	case GLFW_KEY_Q:
		renderer->getCamera().ProcessKeyboard(DOWN, dt);
		break;
	case GLFW_KEY_E:
		renderer->getCamera().ProcessKeyboard(UP, dt);
		break;
	case GLFW_KEY_L:
		renderer->getCamera().ProcessKeyboard(RIGHT, dt, false);
		break;
	case GLFW_KEY_J:
		renderer->getCamera().ProcessKeyboard(LEFT, dt, false);
		break;
	case GLFW_KEY_I:
		renderer->getCamera().ProcessKeyboard(FORWARD, dt, false);
		break;
	case GLFW_KEY_K:
		renderer->getCamera().ProcessKeyboard(BACKWARD, dt, false);
		break;
	case GLFW_KEY_O:
		renderer->getCamera().ProcessKeyboard(UP, dt, false);
		break;
	case GLFW_KEY_U:
		renderer->getCamera().ProcessKeyboard(DOWN, dt, false);
		break;
	case GLFW_KEY_ESCAPE:
		exit(0);
		break;
	default:
		break;
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

	GLFWwindow* window = glfwCreateWindow(700, 700, "Simple CGDemo", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, key_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glfwMakeContextCurrent(window);

	init_opengl();

	renderer.reset(new tScenes());

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	framebuffer_size_callback(window, width, height);
	renderer->init();

	glfwWindowHint(GLFW_SAMPLES, 4);

	while (!glfwWindowShouldClose(window))
	{
		draw_scene();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	exit(EXIT_SUCCESS);
}