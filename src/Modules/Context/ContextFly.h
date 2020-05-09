#pragma once

#include <GLFW/glfw3.h>
#include <memory>
#include <array>

#include <Program/ProgramApi.h>
#include "Context/Context.h"
#include "Context/BaseTypes.h"
#include <Resource/Resource.h>
#include <Instance/Instance.h>
#include <glm/glm.hpp>
#include <gli/gli.hpp>

class ContextFly : public Context
{
public:
    ContextFly(ApiType type, GLFWwindow* window);

protected:
    std::shared_ptr<Instance> m_instance;
    std::shared_ptr<Adapter> m_adapter;
    std::shared_ptr<Device> m_device;
};
