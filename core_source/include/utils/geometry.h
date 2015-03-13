#pragma once

#include <utilities.h>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

struct ModelSuzanne
{
    ModelSuzanne()
    {
        std::string m_path(PROJECT_RESOURCE_MODEL_DIR "suzanne.obj");
		loadOBJ(m_path, vertices, uvs, normals);
    }
    
    std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
}