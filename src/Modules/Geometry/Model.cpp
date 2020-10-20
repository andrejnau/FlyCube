#include "Geometry/Model.h"

Model::Model(Device& device, RenderCommandList& command_list, const std::string& file, uint32_t flags)
    : m_device(device)
    , m_model_loader(std::make_unique<ModelLoader>(file, static_cast<aiPostProcessSteps>(flags), *this))
    , ia(device, command_list, meshes)
    , m_cache(device, command_list)
{
    for (auto & mesh : meshes)
    {
        materials.emplace_back(m_cache, mesh.material, mesh.textures);

        for (const auto& pos : mesh.positions)
        {
            bound_box.x_min = std::min(bound_box.x_min, pos.x);
            bound_box.x_max = std::max(bound_box.x_max, pos.x);
            bound_box.y_min = std::min(bound_box.y_min, pos.y);
            bound_box.y_max = std::max(bound_box.y_max, pos.y);
            bound_box.z_min = std::min(bound_box.z_min, pos.z);
            bound_box.z_max = std::max(bound_box.z_max, pos.z);

            model_center += pos / glm::vec3(mesh.positions.size());
        }
    }
}

void Model::AddMesh(const IMesh& mesh)
{
    meshes.emplace_back(mesh);
}

Bones& Model::GetBones()
{
    return bones;
}
