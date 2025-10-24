#include "RenderUtils/RenderModel.h"

#include "Utilities/FormatHelper.h"
#include "Utilities/NotReached.h"
#include "stb_image.h"

RenderModel::RenderModel(const std::shared_ptr<Device>& device,
                         const std::shared_ptr<CommandQueue>& command_queue,
                         std::unique_ptr<Model> model)
    : RenderModel(device, command_queue, model.get())
{
}

RenderModel::RenderModel(const std::shared_ptr<Device>& device,
                         const std::shared_ptr<CommandQueue>& command_queue,
                         Model* model)
    : m_device(device)
{
    m_command_list = m_device->CreateCommandList(CommandListType::kGraphics);
    m_fence = m_device->CreateFence(m_fence_value);

    m_meshes.resize(model->meshes.size());
    for (int i = 0; i < model->meshes.size(); ++i) {
        m_meshes[i].matrix = model->meshes[i].matrix;

        m_meshes[i].positions = CreateBuffer(BindFlag::kVertexBuffer, model->meshes[i].positions);
        m_meshes[i].normals = CreateBuffer(BindFlag::kVertexBuffer, model->meshes[i].normals);
        m_meshes[i].tangents = CreateBuffer(BindFlag::kVertexBuffer, model->meshes[i].tangents);
        m_meshes[i].texcoords = CreateBuffer(BindFlag::kVertexBuffer, model->meshes[i].texcoords);

        m_meshes[i].index_count = model->meshes[i].indices.size();
        m_meshes[i].index_format = gli::format::FORMAT_R32_UINT_PACK32;
        m_meshes[i].indices = CreateBuffer(BindFlag::kIndexBuffer, model->meshes[i].indices);

        m_meshes[i].textures.base_color = CreateTextureFromFile(model->meshes[i].textures.base_color);
        m_meshes[i].textures.normal = CreateTextureFromFile(model->meshes[i].textures.normal);
        m_meshes[i].textures.metallic_roughness = CreateTextureFromFile(model->meshes[i].textures.metallic_roughness);
        m_meshes[i].textures.ambient_occlusion = CreateTextureFromFile(model->meshes[i].textures.ambient_occlusion);
        m_meshes[i].textures.emissive = CreateTextureFromFile(model->meshes[i].textures.emissive);
    }

    m_command_list->Close();
    command_queue->ExecuteCommandLists({ m_command_list });
    command_queue->Signal(m_fence, ++m_fence_value);
    command_queue->Wait(m_fence, m_fence_value);
}

RenderModel::~RenderModel()
{
    m_fence->Wait(m_fence_value);
}

size_t RenderModel::GetMeshCount() const
{
    return m_meshes.size();
}

const RenderMesh& RenderModel::GetMesh(size_t index) const
{
    return m_meshes.at(index);
}

template <typename T>
std::shared_ptr<Resource> RenderModel::CreateBuffer(uint32_t bind_flag, const std::vector<T>& data)
{
    if (data.empty()) {
        return nullptr;
    }

    std::shared_ptr<Resource> buffer = m_device->CreateBuffer({
        .size = sizeof(data.front()) * data.size(),
        .usage = bind_flag | BindFlag::kCopyDest,
    });
    buffer->CommitMemory(MemoryType::kDefault);
    UpdateSubresource(buffer, /*subresource=*/0, data.data(), 0, 0);
    return buffer;
}

std::shared_ptr<Resource> RenderModel::CreateTextureFromFile(const std::string& path)
{
    std::shared_ptr<Resource> res;
    if (path.empty()) {
        return res;
    }

    std::string full_path = ASSETS_PATH + path;
    if (path.ends_with(".dds") || path.ends_with(".ktx") || path.ends_with(".kmg")) {
        gli::texture texture = gli::load(full_path);

        auto format = texture.format();
        uint32_t width = texture.extent(0).x;
        uint32_t height = texture.extent(0).y;
        size_t mip_levels = texture.levels();

        res =
            m_device->CreateTexture(MemoryType::kDefault, {
                                                              .type = TextureType::k2D,
                                                              .format = format,
                                                              .width = width,
                                                              .height = height,
                                                              .depth_or_array_layers = 1,
                                                              .mip_levels = static_cast<uint32_t>(mip_levels),
                                                              .sample_count = 1,
                                                              .usage = BindFlag::kShaderResource | BindFlag::kCopyDest,
                                                          });
        res->CommitMemory(MemoryType::kDefault);

        for (size_t level = 0; level < mip_levels; ++level) {
            size_t row_bytes = 0;
            size_t num_bytes = 0;
            GetFormatInfo(texture.extent(level).x, texture.extent(level).y, format, num_bytes, row_bytes);
            UpdateSubresource(res, level, texture.data(0, 0, level), row_bytes, num_bytes);
        }
    } else {
        int width = 0;
        int height = 0;
        int comp = 0;
        auto* data = stbi_load(full_path.c_str(), &width, &height, &comp, /*req_comp=*/4);

        res =
            m_device->CreateTexture(MemoryType::kDefault, {
                                                              .type = TextureType::k2D,
                                                              .format = gli::FORMAT_RGBA8_UNORM_PACK8,
                                                              .width = static_cast<uint32_t>(width),
                                                              .height = static_cast<uint32_t>(height),
                                                              .depth_or_array_layers = 1,
                                                              .mip_levels = 1,
                                                              .sample_count = 1,
                                                              .usage = BindFlag::kShaderResource | BindFlag::kCopyDest,
                                                          });

        res->CommitMemory(MemoryType::kDefault);

        size_t row_bytes = 0;
        size_t num_bytes = 0;
        GetFormatInfo(width, height, gli::FORMAT_RGBA8_UNORM_PACK8, num_bytes, row_bytes);
        UpdateSubresource(res, 0, data, row_bytes, num_bytes);

        stbi_image_free(data);
    }
    return res;
}

void RenderModel::UpdateSubresource(const std::shared_ptr<Resource>& resource,
                                    uint32_t subresource,
                                    const void* data,
                                    uint32_t row_pitch,
                                    uint32_t depth_pitch)
{
    switch (resource->GetResourceType()) {
    case ResourceType::kBuffer: {
        size_t buffer_size = resource->GetWidth();
        std::shared_ptr<Resource> upload_resource = m_device->CreateBuffer({
            .size = buffer_size,
            .usage = BindFlag::kCopySource,
        });
        upload_resource->CommitMemory(MemoryType::kUpload);
        upload_resource->UpdateUploadBuffer(0, data, buffer_size);

        std::vector<BufferCopyRegion> regions;
        auto& region = regions.emplace_back();
        region.num_bytes = buffer_size;
        m_command_list->CopyBuffer(upload_resource, resource, regions);
        m_cmd_resources.push_back(upload_resource);
        break;
    }
    case ResourceType::kTexture: {
        std::vector<BufferToTextureCopyRegion> regions;
        auto& region = regions.emplace_back();
        region.texture_mip_level = subresource % resource->GetLevelCount();
        region.texture_array_layer = subresource / resource->GetLevelCount();
        region.texture_extent.width = std::max<uint32_t>(1, resource->GetWidth() >> region.texture_mip_level);
        region.texture_extent.height = std::max<uint32_t>(1, resource->GetHeight() >> region.texture_mip_level);
        region.texture_extent.depth = 1;

        size_t num_bytes = 0, row_bytes = 0, num_rows = 0;
        GetFormatInfo(region.texture_extent.width, region.texture_extent.height, resource->GetFormat(), num_bytes,
                      row_bytes, num_rows, m_device->GetTextureDataPitchAlignment());
        region.buffer_row_pitch = row_bytes;

        std::shared_ptr<Resource> upload_resource = m_device->CreateBuffer({
            .size = num_bytes,
            .usage = BindFlag::kCopySource,
        });
        upload_resource->CommitMemory(MemoryType::kUpload);
        upload_resource->UpdateUploadBufferWithTextureData(0, row_bytes, num_bytes, data, row_pitch, depth_pitch,
                                                           num_rows, region.texture_extent.depth);
        m_command_list->CopyBufferToTexture(upload_resource, resource, regions);
        m_cmd_resources.push_back(upload_resource);
        break;
    }
    default:
        NOTREACHED();
    }
}
