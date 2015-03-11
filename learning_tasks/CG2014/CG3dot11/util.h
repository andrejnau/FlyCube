#pragma once

#include <utility>


#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using line = std::pair<glm::vec2, glm::vec2>;

static float dist(glm::vec2 a, glm::vec2 b)
{
	return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}