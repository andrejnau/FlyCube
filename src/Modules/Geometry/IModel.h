#include "Geometry/IMesh.h"
#include "Geometry/Bones.h"

struct IModel
{
    virtual void AddMesh(const IMesh& mesh) = 0;
    virtual Bones& GetBones() = 0;
};
