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
    bindings_ = reflection_->GetBindings();
    for (uint32_t i = 0; i < bindings_.size(); ++i) {
        BindKey bind_key = { shader_type_, bindings_[i].type, bindings_[i].slot, bindings_[i].space,
                             bindings_[i].count };
        bind_keys_[bindings_[i].name] = bind_key;
        mapping_[bind_key] = i;
        binding_keys_.emplace_back(bind_key);
    }

    decltype(auto) input_parameters = reflection_->GetInputParameters();
    for (uint32_t i = 0; i < input_parameters.size(); ++i) {
        decltype(auto) layout = input_layout_descs_.emplace_back();
        layout.slot = i;
        layout.semantic_name = input_parameters[i].semantic_name;
        layout.format = input_parameters[i].format;
        layout.stride = gli::detail::bits_per_pixel(layout.format) / 8;
        locations_[input_parameters[i].semantic_name] = input_parameters[i].location;
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

const std::vector<ResourceBindingDesc>& ShaderBase::GetResourceBindings() const
{
    return bindings_;
}

const ResourceBindingDesc& ShaderBase::GetResourceBinding(const BindKey& bind_key) const
{
    size_t index = mapping_.at(bind_key);
    return bindings_.at(index);
}

const std::vector<InputLayoutDesc>& ShaderBase::GetInputLayouts() const
{
    return input_layout_descs_;
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
