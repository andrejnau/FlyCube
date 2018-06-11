#include "Geometry/Model.h"

Model::Model(Context& context, const std::string& file, uint32_t flags)
    : m_context(context)
    , m_model_loader(std::make_unique<ModelLoader>(file, (aiPostProcessSteps)flags, *this))
    , ia(context, meshes)
    , m_cache(context)
{
    for (auto & mesh : meshes)
    {
        materials.emplace_back(m_cache, mesh.material, mesh.textures);

        for (auto & pos : mesh.positions)
        {
            bound_box.x_min = std::min(bound_box.x_min, pos.x);
            bound_box.x_max = std::max(bound_box.x_max, pos.x);
            bound_box.y_min = std::min(bound_box.y_min, pos.y);
            bound_box.y_max = std::max(bound_box.y_max, pos.y);
            bound_box.z_min = std::min(bound_box.z_min, pos.z);
            bound_box.z_max = std::max(bound_box.z_max, pos.z);
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
