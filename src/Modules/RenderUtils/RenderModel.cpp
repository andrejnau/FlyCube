#include "RenderUtils/RenderModel.h"

#include "Utilities/Asset.h"
#include "Utilities/FormatHelper.h"
#include "Utilities/NotReached.h"

#include <stb_image.h>

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

    std::shared_ptr<Resource> buffer = m_device->CreateBuffer(
        MemoryType::kDefault, { .size = sizeof(data.front()) * data.size(), .usage = bind_flag | BindFlag::kCopyDest });
    UpdateSubresource(buffer, /*subresource=*/0, data.data(), 0, 0);
    return buffer;
}

std::shared_ptr<Resource> RenderModel::CreateTextureFromFile(const std::string& path)
{
    std::shared_ptr<Resource> texture;
    if (!AssetFileExists(path)) {
        return texture;
    }

    auto file = AssetLoadBinaryFile(path);
    if (path.ends_with(".dds") || path.ends_with(".ktx") || path.ends_with(".kmg")) {
        gli::texture gli_texture = gli::load(reinterpret_cast<char*>(file.data()), file.size());

        TextureDesc texture_desc = {
            .type = TextureType::k2D,
            .format = gli_texture.format(),
            .width = static_cast<uint32_t>(gli_texture.extent(0).x),
            .height = static_cast<uint32_t>(gli_texture.extent(0).y),
            .depth_or_array_layers = 1,
            .mip_levels = static_cast<uint32_t>(gli_texture.levels()),
            .sample_count = 1,
            .usage = BindFlag::kShaderResource | BindFlag::kCopyDest,
        };
        texture = m_device->CreateTexture(MemoryType::kDefault, texture_desc);

        for (size_t level = 0; level < gli_texture.levels(); ++level) {
            size_t row_bytes = 0;
            size_t num_bytes = 0;
            GetFormatInfo(gli_texture.extent(level).x, gli_texture.extent(level).y, gli_texture.format(), num_bytes,
                          row_bytes);
            UpdateSubresource(texture, level, gli_texture.data(0, 0, level), row_bytes, num_bytes);
        }
    } else {
        int width = 0;
        int height = 0;
        int comp = 0;
        auto* data = stbi_load_from_memory(file.data(), file.size(), &width, &height, &comp, /*req_comp=*/4);

        TextureDesc texture_desc = {
            .type = TextureType::k2D,
            .format = gli::FORMAT_RGBA8_UNORM_PACK8,
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height),
            .depth_or_array_layers = 1,
            .mip_levels = 1,
            .sample_count = 1,
            .usage = BindFlag::kShaderResource | BindFlag::kCopyDest,
        };
        texture = m_device->CreateTexture(MemoryType::kDefault, texture_desc);

        size_t row_bytes = 0;
        size_t num_bytes = 0;
        GetFormatInfo(width, height, gli::FORMAT_RGBA8_UNORM_PACK8, num_bytes, row_bytes);
        UpdateSubresource(texture, 0, data, row_bytes, num_bytes);

        stbi_image_free(data);
    }
    return texture;
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
        std::shared_ptr<Resource> upload_buffer =
            m_device->CreateBuffer(MemoryType::kUpload, { .size = buffer_size, .usage = BindFlag::kCopySource });
        upload_buffer->UpdateUploadBuffer(0, data, buffer_size);

        BufferCopyRegion copy_region = {
            .num_bytes = buffer_size,
        };
        m_command_list->CopyBuffer(upload_buffer, resource, { copy_region });
        m_upload_buffers.push_back(upload_buffer);
        break;
    }
    case ResourceType::kTexture: {
        BufferToTextureCopyRegion copy_region = {
            .texture_mip_level = subresource % resource->GetLevelCount(),
            .texture_array_layer = subresource / resource->GetLevelCount(),
            .texture_extent = { .width = std::max<uint32_t>(1, resource->GetWidth() >> copy_region.texture_mip_level),
                                .height = std::max<uint32_t>(1, resource->GetHeight() >> copy_region.texture_mip_level),
                                .depth = 1 },
        };

        size_t num_bytes = 0;
        size_t row_bytes = 0;
        size_t num_rows = 0;
        GetFormatInfo(copy_region.texture_extent.width, copy_region.texture_extent.height, resource->GetFormat(),
                      num_bytes, row_bytes, num_rows, m_device->GetTextureDataPitchAlignment());
        copy_region.buffer_row_pitch = row_bytes;

        std::shared_ptr<Resource> upload_buffer =
            m_device->CreateBuffer(MemoryType::kUpload, { .size = num_bytes, .usage = BindFlag::kCopySource });
        upload_buffer->UpdateUploadBufferWithTextureData(0, row_bytes, num_bytes, data, row_pitch, depth_pitch,
                                                         num_rows, copy_region.texture_extent.depth);
        m_command_list->CopyBufferToTexture(upload_buffer, resource, { copy_region });
        m_upload_buffers.push_back(upload_buffer);
        break;
    }
    default:
        NOTREACHED();
    }
}
