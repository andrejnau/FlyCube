#pragma once

#include "RenderUtils/Model.h"

#include <memory>
#include <string>

std::unique_ptr<Model> LoadModel(const std::string& path);
