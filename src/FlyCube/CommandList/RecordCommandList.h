#pragma once
#include "CommandList/CommandList.h"

#include <deque>
#include <functional>
#include <memory>

template <typename T>
class RecordCommandList : public CommandList {
public:
    RecordCommandList(std::unique_ptr<T> command_list)
        : m_command_list(std::move(command_list))
    {
    }

    void Reset() override
    {
        m_command_list->Reset();
        m_recorded_cmds = {};
        m_executed = false;
    }

    void Close() override
    {
        ApplyAndRecord(&T::Close);
    }

    void BindPipeline(const std::shared_ptr<Pipeline>& state) override
    {
        ApplyAndRecord(&T::BindPipeline, state);
    }

    void BindBindingSet(const std::shared_ptr<BindingSet>& binding_set) override
    {
        ApplyAndRecord(&T::BindBindingSet, binding_set);
    }

    void BeginRenderPass(const std::shared_ptr<RenderPass>& render_pass,
                         const FramebufferDesc& framebuffer_desc,
                         const ClearDesc& clear_desc) override
    {
        ApplyAndRecord(&T::BeginRenderPass, render_pass, framebuffer_desc, clear_desc);
    }

    void EndRenderPass() override
    {
        ApplyAndRecord(&T::EndRenderPass);
    }

    void BeginEvent(const std::string& name) override
    {
        ApplyAndRecord(&T::BeginEvent, name);
    }

    void EndEvent() override
    {
        ApplyAndRecord(&T::EndEvent);
    }

    void Draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) override
    {
        ApplyAndRecord(&T::Draw, vertex_count, instance_count, first_vertex, first_instance);
    }

    void DrawIndexed(uint32_t index_count,
                     uint32_t instance_count,
                     uint32_t first_index,
                     int32_t vertex_offset,
                     uint32_t first_instance) override
    {
        ApplyAndRecord(&T::DrawIndexed, index_count, instance_count, first_index, vertex_offset, first_instance);
    }

    void DrawIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset) override
    {
        ApplyAndRecord(&T::DrawIndirect, argument_buffer, argument_buffer_offset);
    }

    void DrawIndexedIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset) override
    {
        ApplyAndRecord(&T::DrawIndexedIndirect, argument_buffer, argument_buffer_offset);
    }

    void DrawIndirectCount(const std::shared_ptr<Resource>& argument_buffer,
                           uint64_t argument_buffer_offset,
                           const std::shared_ptr<Resource>& count_buffer,
                           uint64_t count_buffer_offset,
                           uint32_t max_draw_count,
                           uint32_t stride) override
    {
        ApplyAndRecord(&T::DrawIndirectCount, argument_buffer, argument_buffer_offset, count_buffer,
                       count_buffer_offset, max_draw_count, stride);
    }

    void DrawIndexedIndirectCount(const std::shared_ptr<Resource>& argument_buffer,
                                  uint64_t argument_buffer_offset,
                                  const std::shared_ptr<Resource>& count_buffer,
                                  uint64_t count_buffer_offset,
                                  uint32_t max_draw_count,
                                  uint32_t stride) override
    {
        ApplyAndRecord(&T::DrawIndexedIndirectCount, argument_buffer, argument_buffer_offset, count_buffer,
                       count_buffer_offset, max_draw_count, stride);
    }

    void Dispatch(uint32_t thread_group_count_x, uint32_t thread_group_count_y, uint32_t thread_group_count_z) override
    {
        ApplyAndRecord(&T::Dispatch, thread_group_count_x, thread_group_count_y, thread_group_count_z);
    }

    void DispatchIndirect(const std::shared_ptr<Resource>& argument_buffer, uint64_t argument_buffer_offset) override
    {
        ApplyAndRecord(&T::DispatchIndirect, argument_buffer, argument_buffer_offset);
    }

    void DispatchMesh(uint32_t thread_group_count_x,
                      uint32_t thread_group_count_y,
                      uint32_t thread_group_count_z) override
    {
        ApplyAndRecord(&T::DispatchMesh, thread_group_count_x, thread_group_count_y, thread_group_count_z);
    }

    void DispatchRays(const RayTracingShaderTables& shader_tables,
                      uint32_t width,
                      uint32_t height,
                      uint32_t depth) override
    {
        ApplyAndRecord(&T::DispatchRays, shader_tables, width, height, depth);
    }

    void ResourceBarrier(const std::vector<ResourceBarrierDesc>& barriers) override
    {
        ApplyAndRecord(&T::ResourceBarrier, barriers);
    }

    void UAVResourceBarrier(const std::shared_ptr<Resource>& resource) override
    {
        ApplyAndRecord(&T::UAVResourceBarrier, resource);
    }

    void SetViewport(float x, float y, float width, float height) override
    {
        ApplyAndRecord(&T::SetViewport, x, y, width, height);
    }

    void SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom) override
    {
        ApplyAndRecord(&T::SetScissorRect, left, top, right, bottom);
    }

    void IASetIndexBuffer(const std::shared_ptr<Resource>& resource, uint64_t offset, gli::format format) override
    {
        ApplyAndRecord(&T::IASetIndexBuffer, resource, offset, format);
    }

    void IASetVertexBuffer(uint32_t slot, const std::shared_ptr<Resource>& resource, uint64_t offset) override
    {
        ApplyAndRecord(&T::IASetVertexBuffer, slot, resource, offset);
    }

    void RSSetShadingRate(ShadingRate shading_rate, const std::array<ShadingRateCombiner, 2>& combiners) override
    {
        ApplyAndRecord(&T::RSSetShadingRate, shading_rate, combiners);
    }

    void BuildBottomLevelAS(const std::shared_ptr<Resource>& src,
                            const std::shared_ptr<Resource>& dst,
                            const std::shared_ptr<Resource>& scratch,
                            uint64_t scratch_offset,
                            const std::vector<RaytracingGeometryDesc>& descs,
                            BuildAccelerationStructureFlags flags) override
    {
        ApplyAndRecord(&T::BuildBottomLevelAS, src, dst, scratch, scratch_offset, descs, flags);
    }

    void BuildTopLevelAS(const std::shared_ptr<Resource>& src,
                         const std::shared_ptr<Resource>& dst,
                         const std::shared_ptr<Resource>& scratch,
                         uint64_t scratch_offset,
                         const std::shared_ptr<Resource>& instance_data,
                         uint64_t instance_offset,
                         uint32_t instance_count,
                         BuildAccelerationStructureFlags flags) override
    {
        ApplyAndRecord(&T::BuildTopLevelAS, src, dst, scratch, scratch_offset, instance_data, instance_offset,
                       instance_count, flags);
    }

    void CopyAccelerationStructure(const std::shared_ptr<Resource>& src,
                                   const std::shared_ptr<Resource>& dst,
                                   CopyAccelerationStructureMode mode) override
    {
        ApplyAndRecord(&T::CopyAccelerationStructure, src, dst, mode);
    }

    void CopyBuffer(const std::shared_ptr<Resource>& src_buffer,
                    const std::shared_ptr<Resource>& dst_buffer,
                    const std::vector<BufferCopyRegion>& regions) override
    {
        ApplyAndRecord(&T::CopyBuffer, src_buffer, dst_buffer, regions);
    }

    void CopyBufferToTexture(const std::shared_ptr<Resource>& src_buffer,
                             const std::shared_ptr<Resource>& dst_texture,
                             const std::vector<BufferToTextureCopyRegion>& regions) override
    {
        ApplyAndRecord(&T::CopyBufferToTexture, src_buffer, dst_texture, regions);
    }

    void CopyTexture(const std::shared_ptr<Resource>& src_texture,
                     const std::shared_ptr<Resource>& dst_texture,
                     const std::vector<TextureCopyRegion>& regions) override
    {
        ApplyAndRecord(&T::CopyTexture, src_texture, dst_texture, regions);
    }

    void WriteAccelerationStructuresProperties(const std::vector<std::shared_ptr<Resource>>& acceleration_structures,
                                               const std::shared_ptr<QueryHeap>& query_heap,
                                               uint32_t first_query) override
    {
        ApplyAndRecord(&T::WriteAccelerationStructuresProperties, acceleration_structures, query_heap, first_query);
    }

    void ResolveQueryData(const std::shared_ptr<QueryHeap>& query_heap,
                          uint32_t first_query,
                          uint32_t query_count,
                          const std::shared_ptr<Resource>& dst_buffer,
                          uint64_t dst_offset) override
    {
        ApplyAndRecord(&T::ResolveQueryData, query_heap, first_query, query_count, dst_buffer, dst_offset);
    }

    void SetName(const std::string& name) override
    {
        ApplyAndRecord(&T::SetName, name);
    }

    T* OnSubmit()
    {
        if (m_executed) {
            m_command_list->Reset();
            for (const auto& cmd : m_recorded_cmds) {
                cmd(m_command_list.get());
            }
        }
        m_executed = true;
        return m_command_list.get();
    }

private:
    template <typename Fn, typename... Args>
    void ApplyAndRecord(Fn&& fn, Args&&... args)
    {
        (m_command_list.get()->*fn)(std::forward<Args>(args)...);
        m_recorded_cmds.push_back([=](T* command_list) { (command_list->*fn)(args...); });
    }

    std::unique_ptr<T> m_command_list;
    std::deque<std::function<void(T*)>> m_recorded_cmds;
    bool m_executed = false;
};
