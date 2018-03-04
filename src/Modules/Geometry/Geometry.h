#pragma once

#include "Geometry/Model.h"

class SceneItem
{
public:
    SceneItem(Context& context, const std::string& file)
        : model(context, file)
    {
    }

    Model model;
    glm::mat4 matrix;
};

using SceneModels = std::vector<SceneItem>;
