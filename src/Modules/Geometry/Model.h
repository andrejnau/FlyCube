#pragma once

#include "Geometry/Mesh.h"
#include "Geometry/Bones.h"
#include "Geometry/ModelLoader.h"

struct BoundBox
{
    BoundBox()
    {
        x_min = y_min = z_min = std::numeric_limits<float>::max();
        x_max = y_max = z_max = std::numeric_limits<float>::min();
    }

    float x_min, y_min, z_min;
    float x_max, y_max, z_max;
};

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
    BoundBox bound_box;
};