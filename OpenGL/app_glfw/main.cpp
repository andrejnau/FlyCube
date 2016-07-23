#include <GLFW/glfw3.h>
#include <scene.h>
#include <state.h>
#include <memory>

struct SceneControl
{
    tScenes::Ptr scene;

    const int base = 100;
    const int init_width = 16 * base;
    const int init_height = 9 * base;

    std::map<int, bool> keys;
    float delta_time = 0.0f;
    double last_frame = 0.0f;
    double last_x = init_width / 2.0;
    double last_y = init_height / 2.0;
    bool first_mouse = true;
} kSceneControl;

tScenes::Ptr& GetScene()
{
    return kSceneControl.scene;
}

Camera& GetCamera()
{
    return GetScene()->getCamera();
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    GetScene()->OnSizeChanged(width, height);
}

void Do_Movement()
{
    Camera &camera = GetCamera();

    if (kSceneControl.keys[GLFW_KEY_W])
        camera.ProcessKeyboard(CameraMovement::kForward, kSceneControl.delta_time);
    if (kSceneControl.keys[GLFW_KEY_S])
        camera.ProcessKeyboard(CameraMovement::kBackward, kSceneControl.delta_time);
    if (kSceneControl.keys[GLFW_KEY_A])
        camera.ProcessKeyboard(CameraMovement::kLeft, kSceneControl.delta_time);
    if (kSceneControl.keys[GLFW_KEY_D])
        camera.ProcessKeyboard(CameraMovement::kRight, kSceneControl.delta_time);
    if (kSceneControl.keys[GLFW_KEY_Q])
        camera.ProcessKeyboard(CameraMovement::kDown, kSceneControl.delta_time);
    if (kSceneControl.keys[GLFW_KEY_E])
        camera.ProcessKeyboard(CameraMovement::kUp, kSceneControl.delta_time);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
        kSceneControl.keys[key] = true;
    else if (action == GLFW_RELEASE)
        kSceneControl.keys[key] = false;

    if (kSceneControl.keys[GLFW_KEY_ESCAPE])
        exit(0);

    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        auto & state = CurState<bool>::Instance().state;
        state["warframe"] = !state["warframe"];
    }

    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        auto & state = CurState<bool>::Instance().state;
        state["occlusion"] = !state["occlusion"];
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        auto & state = CurState<bool>::Instance().state;
        state["pause"] = !state["pause"];
    }

    if (key == GLFW_KEY_L && action == GLFW_PRESS)
    {
        auto & state = CurState<bool>::Instance().state;
        state["shadow"] = !state["shadow"];
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (kSceneControl.first_mouse)
    {
        kSceneControl.last_x = xpos;
        kSceneControl.last_y = ypos;
        kSceneControl.first_mouse = false;
    }

    double xoffset = xpos - kSceneControl.last_x;
    double yoffset = kSceneControl.last_y - ypos;  // Reversed since y-coordinates go from bottom to left

    kSceneControl.last_x = xpos;
    kSceneControl.last_y = ypos;

    GetCamera().ProcessMouseMovement((float)xoffset, (float)yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    GetCamera().ProcessMouseScroll((float)yoffset);
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void APIENTRY gl_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *msg, const void *data)
{
    std::cout << "debug call: " << msg << std::endl;
}

void setWindowOnCenter(GLFWwindow* window)
{
    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    int monitor_width, monitor_height;
    const GLFWvidmode *monitor_vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    monitor_width = monitor_vidmode->width;
    monitor_height = monitor_vidmode->height;

    glfwSetWindowPos(window,
        (monitor_width - window_width) / 2,
        (monitor_height - window_height) / 2);
}

int main(void)
{
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        return EXIT_FAILURE;

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

    GLFWwindow *window = glfwCreateWindow(kSceneControl.init_width, kSceneControl.init_height, "[OpenGL] testApp", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    setWindowOnCenter(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwMakeContextCurrent(window);

    ogl_LoadFunctions();
    glDebugMessageCallback(gl_callback, nullptr);

    auto & state = CurState<bool>::Instance().state;
    state["occlusion"] = true;
    state["shadow"] = true;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    GetScene().reset(new tScenes(width, height));
    GetScene()->OnInit();

    double lastTime = glfwGetTime();
    int nbFrames = 0;
    while (!glfwWindowShouldClose(window))
    {
        double currentFrame = glfwGetTime();

        kSceneControl.delta_time = (float)(currentFrame - kSceneControl.last_frame);
        kSceneControl.last_frame = currentFrame;

        glfwPollEvents();
        Do_Movement();

        GetScene()->OnUpdate();
        GetScene()->OnRender();

        glfwSwapBuffers(window);

        nbFrames++;
        double delta = currentFrame - lastTime;
        if (delta >= 1.0)
        {
            double fps = double(nbFrames) / delta;

            std::stringstream ss;
            ss << "[OpenGL] testApp" <<  " [" << fps << " FPS]";

            glfwSetWindowTitle(window, ss.str().c_str());

            nbFrames = 0;
            lastTime = currentFrame;
        }
    }

    GetScene()->OnDestroy();

    return EXIT_SUCCESS;
}