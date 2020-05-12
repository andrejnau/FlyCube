#pragma once
#include <Resource/Resource.h>
#include <View/View.h>
#include <PipelineState/PipelineState.h>
#include <Instance/BaseTypes.h>
#include <memory>
#include <array>
#include <gli/gli.hpp>

class CommandList
{
public:
    virtual ~CommandList() = default;
    virtual void Open() = 0;
    virtual void Close() = 0;
    virtual void BindPipelineState(const std::shared_ptr<PipelineState>& state) = 0;
    virtual void Clear(const std::shared_ptr<View>& view, const std::array<float, 4>& color) = 0;
    virtual void DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location) = 0;
    virtual void ResourceBarrier(const std::shared_ptr<Resource>& resource, ResourceState state) = 0;
    virtual void SetViewport(float width, float height) = 0;
    virtual void IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format) = 0;
    virtual void IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource) = 0;
    virtual void UpdateSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch = 0, uint32_t depth_pitch = 0) = 0;
};
