#include "Geometry/Model.h"

Model::Model(Context& context, const std::string& file, uint32_t flags)
    : m_context(context)
    , m_model_loader(std::make_unique<ModelLoader>(file, (aiPostProcessSteps)flags, *this))
    , ia(context, meshes)
{
    for (auto & mesh : meshes)
    {
        materials.emplace_back(context, mesh.material, mesh.textures);
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
