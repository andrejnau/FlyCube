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
    virtual void BeginEvent(const std::string& name) = 0;
    virtual void EndEvent() = 0;
    virtual void ClearColor(const std::shared_ptr<View>& view, const std::array<float, 4>& color) = 0;
    virtual void ClearDepth(const std::shared_ptr<View>& view, float depth) = 0;
    virtual void DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location) = 0;
    virtual void Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z) = 0;
    virtual void DispatchRays(uint32_t width, uint32_t height, uint32_t depth) = 0;
    virtual void ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers) = 0;
    virtual void SetViewport(float width, float height, bool set_scissor = true) = 0;
    virtual void SetScissorRect(int32_t left, int32_t top, uint32_t right, uint32_t bottom) = 0;
    virtual void IASetIndexBuffer(const std::shared_ptr<Resource>& resource, gli::format format) = 0;
    virtual void IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource) = 0;
    virtual void RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners = {}) = 0;
    virtual void RSSetShadingRateImage(const std::shared_ptr<Resource>& resource) = 0;
    virtual void BuildBottomLevelAS(const std::shared_ptr<Resource>& result, const std::shared_ptr<Resource>& scratch, const BufferDesc& vertex, const BufferDesc& index) = 0;
    virtual void BuildTopLevelAS(const std::shared_ptr<Resource>& result, const std::shared_ptr<Resource>& scratch, const std::shared_ptr<Resource>& instance_data, uint32_t instance_count) = 0;
    virtual void CopyBuffer(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_buffer,
                            const std::vector<BufferCopyRegion>& regions) = 0;
    virtual void CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer, const std::shared_ptr<Resource>& dst_texture,
                                     const std::vector<BufferToTextureCopyRegion>& regions) = 0;
};
