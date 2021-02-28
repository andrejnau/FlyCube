#pragma once
#include <Instance/QueryInterface.h>
#include <Instance/BaseTypes.h>
#include <memory>
#include <string>
#include <vector>

enum class ShaderKind
{
    kUnknown = 0,
    kPixel,
    kVertex,
    kGeometry,
    kCompute,
    kLibrary,
    kRayGeneration,
    kIntersection,
    kAnyHit,
    kClosestHit,
    kMiss,
    kCallable,
    kMesh,
    kAmplification,
};

struct EntryPoint
{
    std::string name;
    ShaderKind kind;
};

inline bool operator== (const EntryPoint& lhs, const EntryPoint& rhs)
{
    return std::tie(lhs.name, lhs.kind) == std::tie(rhs.name, rhs.kind);
}

inline auto MakeTie(const ResourceBindingDesc& desc)
{
    return std::tie(desc.name, desc.type, desc.slot, desc.space, desc.dimension);
};

inline bool operator== (const ResourceBindingDesc& lhs, const ResourceBindingDesc& rhs)
{
    return MakeTie(lhs) == MakeTie(rhs);
}

inline bool operator< (const ResourceBindingDesc& lhs, const ResourceBindingDesc& rhs)
{
    return MakeTie(lhs) < MakeTie(rhs);
}

struct InputParameterDesc
{
    uint32_t location;
    std::string semantic_name;
    gli::format format;
};

class ShaderReflection : public QueryInterface
{
public:
    virtual ~ShaderReflection() = default;
    virtual const std::vector<EntryPoint> GetEntryPoints() const = 0;
    virtual const std::vector<ResourceBindingDesc> GetBindings() const = 0;
    virtual const std::vector<InputParameterDesc> GetInputParameters() const = 0;
};

std::shared_ptr<ShaderReflection> CreateShaderReflection(ShaderBlobType type, const void* data, size_t size);
