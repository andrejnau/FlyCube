#pragma once
#include "Instance/BaseTypes.h"
#include "Instance/QueryInterface.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

enum class ShaderKind {
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

struct EntryPoint {
    std::string name;
    ShaderKind kind;
    uint32_t payload_size;
    uint32_t attribute_size;
};

struct InputParameterDesc {
    uint32_t location;
    std::string semantic_name;
    gli::format format;
};

struct OutputParameterDesc {
    uint32_t slot;
};

enum class VariableType {
    kStruct,
    kFloat,
    kInt,
    kUint,
    kBool,
};

struct VariableLayout {
    std::string name;
    VariableType type;
    uint32_t offset;
    uint32_t size;
    uint32_t rows;
    uint32_t columns;
    uint32_t elements;
    std::vector<VariableLayout> members;
};

struct ShaderFeatureInfo {
    bool resource_descriptor_heap_indexing = false;
    bool sampler_descriptor_heap_indexing = false;
    std::array<uint32_t, 3> numthreads = {};
};

class ShaderReflection : public QueryInterface {
public:
    virtual ~ShaderReflection() = default;
    virtual const std::vector<EntryPoint>& GetEntryPoints() const = 0;
    virtual const std::vector<ResourceBindingDesc>& GetBindings() const = 0;
    virtual const std::vector<VariableLayout>& GetVariableLayouts() const = 0;
    virtual const std::vector<InputParameterDesc>& GetInputParameters() const = 0;
    virtual const std::vector<OutputParameterDesc>& GetOutputParameters() const = 0;
    virtual const ShaderFeatureInfo& GetShaderFeatureInfo() const = 0;
};

std::shared_ptr<ShaderReflection> CreateShaderReflection(ShaderBlobType type, const void* data, size_t size);
