#include "BindingSetLayout/VKBindingSetLayout.h"

#include "Device/VKDevice.h"

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
        break;
    }
    assert(false);
    return {};
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
        return vk::ShaderStageFlagBits::eTaskNV;
    case ShaderType::kMesh:
        return vk::ShaderStageFlagBits::eMeshNV;
    case ShaderType::kLibrary:
        return vk::ShaderStageFlagBits::eAll;
    default:
        assert(false);
        return {};
    }
}

VKBindingSetLayout::VKBindingSetLayout(VKDevice& device, const std::vector<BindKey>& descs)
{
    std::map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>> bindings_by_set;
    std::map<uint32_t, std::vector<vk::DescriptorBindingFlags>> bindings_flags_by_set;

    for (const auto& bind_key : descs) {
        decltype(auto) binding = bindings_by_set[bind_key.space].emplace_back();
        binding.binding = bind_key.slot;
        binding.descriptorType = GetDescriptorType(bind_key.view_type);
        binding.descriptorCount = bind_key.count;
        binding.stageFlags = ShaderType2Bit(bind_key.shader_type);

        decltype(auto) binding_flag = bindings_flags_by_set[bind_key.space].emplace_back();
        if (bind_key.count == std::numeric_limits<uint32_t>::max()) {
            binding.descriptorCount = device.GetMaxDescriptorSetBindings(binding.descriptorType);
            binding_flag = vk::DescriptorBindingFlagBits::eVariableDescriptorCount;
            m_bindless_type.emplace(bind_key.space, binding.descriptorType);
            binding.stageFlags = vk::ShaderStageFlagBits::eAll;
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
        if (m_descriptor_set_layouts.size() <= set_num) {
            m_descriptor_set_layouts.resize(set_num + 1);
            m_descriptor_count_by_set.resize(set_num + 1);
        }

        decltype(auto) descriptor_set_layout = m_descriptor_set_layouts[set_num];
        descriptor_set_layout = device.GetDevice().createDescriptorSetLayoutUnique(layout_info);

        decltype(auto) descriptor_count = m_descriptor_count_by_set[set_num];
        for (const auto& binding : set_desc.second) {
            descriptor_count[binding.descriptorType] += binding.descriptorCount;
        }
    }

    std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
    for (auto& descriptor_set_layout : m_descriptor_set_layouts) {
        if (!descriptor_set_layout) {
            vk::DescriptorSetLayoutCreateInfo layout_info = {};
            descriptor_set_layout = device.GetDevice().createDescriptorSetLayoutUnique(layout_info);
        }

        descriptor_set_layouts.emplace_back(descriptor_set_layout.get());
    }

    vk::PipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.setLayoutCount = descriptor_set_layouts.size();
    pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();

    m_pipeline_layout = device.GetDevice().createPipelineLayoutUnique(pipeline_layout_info);
}

const std::map<uint32_t, vk::DescriptorType>& VKBindingSetLayout::GetBindlessType() const
{
    return m_bindless_type;
}

const std::vector<vk::UniqueDescriptorSetLayout>& VKBindingSetLayout::GetDescriptorSetLayouts() const
{
    return m_descriptor_set_layouts;
}

const std::vector<std::map<vk::DescriptorType, size_t>>& VKBindingSetLayout::GetDescriptorCountBySet() const
{
    return m_descriptor_count_by_set;
}

vk::PipelineLayout VKBindingSetLayout::GetPipelineLayout() const
{
    return m_pipeline_layout.get();
}
