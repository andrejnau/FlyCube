#pragma once
#include <Instance/QueryInterface.h>
#include <memory>
#include <string>
#include <vector>

enum class ShaderBlobType
{
    kDXIL,
    kSPIRV
};

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

class ShaderReflection : public QueryInterface
{
public:
    virtual ~ShaderReflection() = default;
    virtual const std::vector<EntryPoint> GetEntryPoints() const = 0;
};

std::shared_ptr<ShaderReflection> CreateShaderReflection(ShaderBlobType type, const void* data, size_t size);
