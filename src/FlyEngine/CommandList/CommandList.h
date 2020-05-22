#pragma once
#include <Instance/QueryInterface.h>
#include <Resource/Resource.h>
#include <View/View.h>
#include <Pipeline/Pipeline.h>
#include <Instance/BaseTypes.h>
#include <Framebuffer/Framebuffer.h>
#include <BindingSet/BindingSet.h>
#include <memory>
#include <array>
#include <gli/format.hpp>

class CommandList : public QueryInterface
{
public:
    virtual ~CommandList() = default;
    virtual void Open() = 0;
    virtual void Close() = 0;
    virtual void BindPipeline(const std::shared_ptr<Pipeline>& state) = 0;
    virtual void BindBindingSet(const std::shared_ptr<BindingSet>& binding_set) = 0;
    virtual void BeginRenderPass(const std::shared_ptr<Framebuffer>& framebuffer) = 0;
    virtual void EndRenderPass() = 0;
    virtual void Clear(const std::shared_ptr<View>& view, const std::array<float, 4>& color) = 0;
    virtual void ClearDepth(const std::shared_ptr<View>& view, float depth) = 0;
    virtual void DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location) = 0;
    virtual void Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z) = 0;
    virtual void ResourceBarrier(const std::shared_ptr<Resource>& resource, ResourceState state) = 0;
    virtual void SetViewport(float width, float height) = 0;
    virtual void IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format) = 0;
    virtual void IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource) = 0;
    virtual void UpdateSubresource(const std::shared_ptr<Resource>& resource, uint32_t subresource, const void* data, uint32_t row_pitch = 0, uint32_t depth_pitch = 0) = 0;
};
