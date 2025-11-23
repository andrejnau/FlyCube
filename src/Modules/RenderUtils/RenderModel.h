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

    struct Buffer {
        std::shared_ptr<Resource> buffer;
        uint64_t offset = 0;
    };

    Buffer indices;
    Buffer positions;
    Buffer normals;
    Buffer tangents;
    Buffer texcoords;

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
    std::shared_ptr<Resource> CreateUploadBuffer(const BufferDesc& desc);
    std::shared_ptr<Resource> CreateTextureFromFile(const std::string& path);
    void UpdateTexture(const std::shared_ptr<Resource>& texture,
                       uint32_t mip_level,
                       uint32_t array_layer,
                       uint32_t width,
                       uint32_t height,
                       const void* data,
                       uint64_t row_pitch,
                       uint64_t slice_pitch,
                       uint32_t num_rows);

    std::shared_ptr<Device> device_;
    std::shared_ptr<CommandList> command_list_;
    std::vector<std::shared_ptr<Resource>> upload_buffers_;
    std::shared_ptr<Fence> fence_;
    uint64_t fence_value_ = 0;
    std::vector<RenderMesh> meshes_;
};
