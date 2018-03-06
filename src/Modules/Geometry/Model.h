#pragma once

#include "Geometry/Mesh.h"
#include "Geometry/Bones.h"
#include "Geometry/ModelLoader.h"

class Model : public IModel
{
public:
    Model(Context& context, const std::string& file, uint32_t flags = ~0);
    virtual void AddMesh(const IMesh& mesh) override;
    virtual Bones& GetBones() override;

    std::vector<IMesh> meshes;
    Bones bones;

    glm::mat4 matrix;

    const Material& GetMaterial(size_t range_id) const
    {
        return materials[range_id];
    }

private:
    Context& m_context;
    std::unique_ptr<ModelLoader> m_model_loader;

public:
    IAMergedMesh ia;
    std::vector<Material> materials;
};
