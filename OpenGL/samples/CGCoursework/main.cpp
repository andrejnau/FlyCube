#include <GLFW/glfw3.h>
#include <scene.h>
#include <state.h>
#include <memory>

std::unique_ptr<tScenes> renderer;
std::map<int, bool> keys;
float deltaTime = 0.0f;
double lastFrame = 0.0f;
double lastX = 400, lastY = 300;
bool firstMouse = true;

void draw_scene()
{
    renderer->draw();
}

void init_scene(int width, int height)
{
    ogl_LoadFunctions();
    renderer.reset(new tScenes());
    renderer->resize(0, 0, width, height);
    renderer->init();
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    renderer->resize(0, 0, width, height);
}

void Do_Movement()
{
    auto &camera = renderer->getCamera();

    if (keys[GLFW_KEY_W])
        camera.ProcessKeyboard(CameraMovement::kForward, deltaTime);
    if (keys[GLFW_KEY_S])
        camera.ProcessKeyboard(CameraMovement::kBackward, deltaTime);
    if (keys[GLFW_KEY_A])
        camera.ProcessKeyboard(CameraMovement::kLeft, deltaTime);
    if (keys[GLFW_KEY_D])
        camera.ProcessKeyboard(CameraMovement::kRight, deltaTime);
    if (keys[GLFW_KEY_Q])
        camera.ProcessKeyboard(CameraMovement::kDown, deltaTime);
    if (keys[GLFW_KEY_E])
        camera.ProcessKeyboard(CameraMovement::kUp, deltaTime);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
        keys[key] = true;
    else if (action == GLFW_RELEASE)
        keys[key] = false;

    if (keys[GLFW_KEY_ESCAPE])
        exit(0);

    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        auto & state = CurState<bool>::Instance().state;
        state["warframe"] = !state["warframe"];
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    auto &camera = renderer->getCamera();

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos;  // Reversed since y-coordinates go from bottom to left

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement((float)xoffset, (float)yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto &camera = renderer->getCamera();
    camera.ProcessMouseScroll((float)yoffset);
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

    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Simple Demo", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwMakeContextCurrent(window);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    init_scene(width, height);

    while (!glfwWindowShouldClose(window))
    {
        double currentFrame = glfwGetTime();
        deltaTime = (float)(currentFrame - lastFrame);
        lastFrame = currentFrame;

        glfwPollEvents();
        Do_Movement();

        draw_scene();

        glfwSwapBuffers(window);
    }

    exit(EXIT_SUCCESS);
}