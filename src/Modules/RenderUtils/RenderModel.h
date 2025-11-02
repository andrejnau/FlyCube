#pragma once
#include "Instance/Instance.h"
#include "RenderUtils/Model.h"

#include <gli/gli.hpp>
#include <glm/glm.hpp>

#include <memory>
#include <vector>

struct RenderMesh {
    glm::mat4 matrix = glm::mat4(1.0);
    size_t index_count = 0;
    gli::format index_format = gli::format::FORMAT_UNDEFINED;

    std::shared_ptr<Resource> indices;
    std::shared_ptr<Resource> positions;
    std::shared_ptr<Resource> normals;
    std::shared_ptr<Resource> tangents;
    std::shared_ptr<Resource> texcoords;

    struct {
        std::shared_ptr<Resource> base_color;
        std::shared_ptr<Resource> normal;
        std::shared_ptr<Resource> metallic_roughness;
        std::shared_ptr<Resource> ambient_occlusion;
        std::shared_ptr<Resource> emissive;
    } textures;
};

class RenderModel {
public:
    RenderModel() = default;
    RenderModel(const std::shared_ptr<Device>& device,
                const std::shared_ptr<CommandQueue>& command_queue,
                std::unique_ptr<Model> model);
    RenderModel(const std::shared_ptr<Device>& device,
                const std::shared_ptr<CommandQueue>& command_queue,
                Model* model);
    ~RenderModel();

    size_t GetMeshCount() const;
    const RenderMesh& GetMesh(size_t index) const;

private:
    template <typename T>
    std::shared_ptr<Resource> CreateBuffer(uint32_t bind_flag, const std::vector<T>& data);
    std::shared_ptr<Resource> CreateTextureFromFile(const std::string& path);
    void UpdateSubresource(const std::shared_ptr<Resource>& resource,
                           uint32_t subresource,
                           const void* data,
                           uint32_t row_pitch,
                           uint32_t depth_pitch);

    std::shared_ptr<Device> m_device;
    std::shared_ptr<CommandList> m_command_list;
    std::vector<std::shared_ptr<Resource>> m_upload_buffers;
    std::shared_ptr<Fence> m_fence;
    uint64_t m_fence_value = 0;
    std::vector<RenderMesh> m_meshes;
};
