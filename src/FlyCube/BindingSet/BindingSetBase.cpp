#include "BindingSet/BindingSetBase.h"

#include "Device/Device.h"

void BindingSetBase::CreateConstantsFallbackBuffer(Device& device, const std::vector<BindingConstants>& constants)
{
    uint64_t num_bytes = 0;
    for (const auto& [bind_key, size] : constants) {
        num_bytes += size;
    }

    fallback_constants_buffer_ =
        device.CreateBuffer(MemoryType::kUpload, { .size = num_bytes, .usage = BindFlag::kConstantBuffer });

    num_bytes = 0;
    for (const auto& [bind_key, size] : constants) {
        fallback_constants_buffer_offsets_[bind_key] = num_bytes;
        ViewDesc view_desc = {
            .view_type = ViewType::kConstantBuffer,
            .dimension = ViewDimension::kBuffer,
            .offset = num_bytes,
        };
        fallback_constants_buffer_views_[bind_key] = device.CreateView(fallback_constants_buffer_, view_desc);
        num_bytes += size;
    }
}

void BindingSetBase::UpdateConstantsFallbackBuffer(const std::vector<BindingConstantsData>& constants)
{
    for (const auto& [bind_key, data] : constants) {
        fallback_constants_buffer_->UpdateUploadBuffer(fallback_constants_buffer_offsets_.at(bind_key), data.data(),
                                                       data.size());
    }
}
