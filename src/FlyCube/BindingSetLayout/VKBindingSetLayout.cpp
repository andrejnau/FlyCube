#include "BindingSetLayout/VKBindingSetLayout.h"

#include "Device/VKDevice.h"
#include "Utilities/NotReached.h"

#include <set>

vk::DescriptorType GetDescriptorType(ViewType view_type)
{
    switch (view_type) {
    case ViewType::kConstantBuffer:
        return vk::DescriptorType::eUniformBuffer;
    case ViewType::kSampler:
        return vk::DescriptorType::eSampler;
    case ViewType::kTexture:
        return vk::DescriptorType::eSampledImage;
    case ViewType::kRWTexture:
        return vk::DescriptorType::eStorageImage;
    case ViewType::kBuffer:
        return vk::DescriptorType::eUniformTexelBuffer;
    case ViewType::kRWBuffer:
        return vk::DescriptorType::eStorageTexelBuffer;
    case ViewType::kStructuredBuffer:
        return vk::DescriptorType::eStorageBuffer;
    case ViewType::kRWStructuredBuffer:
        return vk::DescriptorType::eStorageBuffer;
    case ViewType::kAccelerationStructure:
        return vk::DescriptorType::eAccelerationStructureKHR;
    default:
        NOTREACHED();
    }
}

vk::ShaderStageFlagBits ShaderType2Bit(ShaderType type)
{
    switch (type) {
    case ShaderType::kVertex:
        return vk::ShaderStageFlagBits::eVertex;
    case ShaderType::kPixel:
        return vk::ShaderStageFlagBits::eFragment;
    case ShaderType::kGeometry:
        return vk::ShaderStageFlagBits::eGeometry;
    case ShaderType::kCompute:
        return vk::ShaderStageFlagBits::eCompute;
    case ShaderType::kAmplification:
        return vk::ShaderStageFlagBits::eTaskEXT;
    case ShaderType::kMesh:
        return vk::ShaderStageFlagBits::eMeshEXT;
    case ShaderType::kLibrary:
        return vk::ShaderStageFlagBits::eAll;
    default:
        NOTREACHED();
    }
}

VKBindingSetLayout::VKBindingSetLayout(VKDevice& device,
                                       const std::vector<BindKey>& bind_keys,
                                       const std::vector<BindingConstants>& constants)
{
    std::map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>> bindings_by_set;
    std::map<uint32_t, std::vector<vk::DescriptorBindingFlags>> bindings_flags_by_set;
    std::map<uint32_t, std::set<uint32_t>> used_bindings_by_set;

    for (const auto& bind_key : bind_keys) {
        assert(!used_bindings_by_set[bind_key.space].contains(bind_key.slot));
        used_bindings_by_set[bind_key.space].insert(bind_key.slot);

        decltype(auto) binding = bindings_by_set[bind_key.space].emplace_back();
        binding.binding = bind_key.slot;
        binding.descriptorType = GetDescriptorType(bind_key.view_type);
        binding.descriptorCount = bind_key.count;
        binding.stageFlags = ShaderType2Bit(bind_key.shader_type);

        decltype(auto) binding_flag = bindings_flags_by_set[bind_key.space].emplace_back();
        if (bind_key.count == kBindlessCount) {
            binding.descriptorCount = device.GetMaxDescriptorSetBindings(binding.descriptorType);
            binding_flag = vk::DescriptorBindingFlagBits::eVariableDescriptorCount;
            bindless_type_.emplace(bind_key.space, binding.descriptorType);
            binding.stageFlags = vk::ShaderStageFlagBits::eAll;
        } else {
            binding_flag = vk::DescriptorBindingFlagBits::ePartiallyBound;
        }
    }

    for (const auto& set_desc : bindings_by_set) {
        vk::DescriptorSetLayoutCreateInfo layout_info = {};
        layout_info.bindingCount = set_desc.second.size();
        layout_info.pBindings = set_desc.second.data();

        vk::DescriptorSetLayoutBindingFlagsCreateInfo layout_flags_info = {};
        layout_flags_info.bindingCount = bindings_flags_by_set[set_desc.first].size();
        layout_flags_info.pBindingFlags = bindings_flags_by_set[set_desc.first].data();
        layout_info.pNext = &layout_flags_info;

        size_t set_num = set_desc.first;
        if (descriptor_set_layouts_.size() <= set_num) {
            descriptor_set_layouts_.resize(set_num + 1);
            descriptor_count_by_set_.resize(set_num + 1);
        }

        decltype(auto) descriptor_set_layout = descriptor_set_layouts_[set_num];
        descriptor_set_layout = device.GetDevice().createDescriptorSetLayoutUnique(layout_info);

        decltype(auto) descriptor_count = descriptor_count_by_set_[set_num];
        for (const auto& binding : set_desc.second) {
            descriptor_count[binding.descriptorType] += binding.descriptorCount;
        }
    }

    std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
    for (auto& descriptor_set_layout : descriptor_set_layouts_) {
        if (!descriptor_set_layout) {
            vk::DescriptorSetLayoutCreateInfo layout_info = {};
            descriptor_set_layout = device.GetDevice().createDescriptorSetLayoutUnique(layout_info);
        }

        descriptor_set_layouts.emplace_back(descriptor_set_layout.get());
    }

    vk::PipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.setLayoutCount = descriptor_set_layouts.size();
    pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();

    pipeline_layout_ = device.GetDevice().createPipelineLayoutUnique(pipeline_layout_info);
}

const std::map<uint32_t, vk::DescriptorType>& VKBindingSetLayout::GetBindlessType() const
{
    return bindless_type_;
}

const std::vector<vk::UniqueDescriptorSetLayout>& VKBindingSetLayout::GetDescriptorSetLayouts() const
{
    return descriptor_set_layouts_;
}

const std::vector<std::map<vk::DescriptorType, size_t>>& VKBindingSetLayout::GetDescriptorCountBySet() const
{
    return descriptor_count_by_set_;
}

vk::PipelineLayout VKBindingSetLayout::GetPipelineLayout() const
{
    return pipeline_layout_.get();
}
