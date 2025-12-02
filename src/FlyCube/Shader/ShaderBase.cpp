#include "Shader/ShaderBase.h"

namespace {

uint64_t GenId()
{
    static uint64_t id = 0;
    return ++id;
}

} // namespace

ShaderBase::ShaderBase(const std::vector<uint8_t>& blob, ShaderBlobType blob_type, ShaderType shader_type)
    : blob_(blob)
    , blob_type_(blob_type)
    , shader_type_(shader_type)
{
    reflection_ = CreateShaderReflection(blob_type, blob_.data(), blob_.size());
    for (const auto& binding : reflection_->GetBindings()) {
        BindKey bind_key = {
            .shader_type = shader_type_,
            .view_type = binding.type,
            .slot = binding.slot,
            .space = binding.space,
            .count = binding.count,
        };
        bind_keys_[binding.name] = bind_key;
        binding_keys_.emplace_back(bind_key);
    }

    for (const auto& input_parameter : reflection_->GetInputParameters()) {
        locations_[input_parameter.semantic_name] = input_parameter.location;
    }

    for (const auto& entry_point : reflection_->GetEntryPoints()) {
        ids_.emplace(entry_point.name, GenId());
    }
}

ShaderType ShaderBase::GetType() const
{
    return shader_type_;
}

const std::vector<uint8_t>& ShaderBase::GetBlob() const
{
    return blob_;
}

uint64_t ShaderBase::GetId(const std::string& entry_point) const
{
    return ids_.at(entry_point);
}

const BindKey& ShaderBase::GetBindKey(const std::string& name) const
{
    return bind_keys_.at(name);
}

uint32_t ShaderBase::GetInputLayoutLocation(const std::string& semantic_name) const
{
    return locations_.at(semantic_name);
}

const std::vector<BindKey>& ShaderBase::GetBindings() const
{
    return binding_keys_;
}

const std::shared_ptr<ShaderReflection>& ShaderBase::GetReflection() const
{
    return reflection_;
}
