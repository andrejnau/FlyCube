#include "RenderUtils/RenderModel.h"

#include "Utilities/Asset.h"
#include "Utilities/Common.h"
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
    : device_(device)
{
    command_list_ = device_->CreateCommandList(CommandListType::kGraphics);
    fence_ = device_->CreateFence(fence_value_);

    meshes_.resize(model->meshes.size());
    for (int i = 0; i < model->meshes.size(); ++i) {
        meshes_[i].matrix = model->meshes[i].matrix;

        meshes_[i].positions.buffer = CreateBuffer(BindFlag::kVertexBuffer, model->meshes[i].positions);
        meshes_[i].normals.buffer = CreateBuffer(BindFlag::kVertexBuffer, model->meshes[i].normals);
        meshes_[i].tangents.buffer = CreateBuffer(BindFlag::kVertexBuffer, model->meshes[i].tangents);
        meshes_[i].texcoords.buffer = CreateBuffer(BindFlag::kVertexBuffer, model->meshes[i].texcoords);

        meshes_[i].index_count = model->meshes[i].indices.size();
        meshes_[i].index_format = gli::format::FORMAT_R32_UINT_PACK32;
        meshes_[i].indices.buffer = CreateBuffer(BindFlag::kIndexBuffer, model->meshes[i].indices);

        meshes_[i].textures.base_color = CreateTextureFromFile(model->meshes[i].textures.base_color);
        meshes_[i].textures.normal = CreateTextureFromFile(model->meshes[i].textures.normal);
        meshes_[i].textures.metallic_roughness = CreateTextureFromFile(model->meshes[i].textures.metallic_roughness);
        meshes_[i].textures.ambient_occlusion = CreateTextureFromFile(model->meshes[i].textures.ambient_occlusion);
        meshes_[i].textures.emissive = CreateTextureFromFile(model->meshes[i].textures.emissive);
    }

    command_list_->Close();
    command_queue->ExecuteCommandLists({ command_list_ });
    command_queue->Signal(fence_, ++fence_value_);
    command_queue->Wait(fence_, fence_value_);
}

RenderModel::~RenderModel()
{
    fence_->Wait(fence_value_);
}

size_t RenderModel::GetMeshCount() const
{
    return meshes_.size();
}

const RenderMesh& RenderModel::GetMesh(size_t index) const
{
    return meshes_.at(index);
}

template <typename T>
std::shared_ptr<Resource> RenderModel::CreateBuffer(uint32_t bind_flag, const std::vector<T>& data)
{
    if (data.empty()) {
        return nullptr;
    }

    uint64_t num_bytes = sizeof(data.front()) * data.size();
    std::shared_ptr<Resource> buffer =
        device_->CreateBuffer(MemoryType::kDefault, { .size = num_bytes, .usage = bind_flag | BindFlag::kCopyDest });
    UpdateBuffer(buffer, /*buffer_offset=*/0, data.data(), num_bytes);
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
        texture = device_->CreateTexture(MemoryType::kDefault, texture_desc);

        for (size_t level = 0; level < gli_texture.levels(); ++level) {
            auto extent = gli_texture.extent(level);
            size_t row_bytes = 0;
            size_t num_bytes = 0;
            size_t num_rows = 0;
            GetFormatInfo(extent.x, extent.y, gli_texture.format(), num_bytes, row_bytes, num_rows);
            UpdateTexture(texture, level, 0, extent.x, extent.y, gli_texture.data(0, 0, level), row_bytes, num_bytes,
                          num_rows);
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
        texture = device_->CreateTexture(MemoryType::kDefault, texture_desc);

        size_t row_bytes = 0;
        size_t num_bytes = 0;
        size_t num_rows = 0;
        GetFormatInfo(width, height, gli::FORMAT_RGBA8_UNORM_PACK8, num_bytes, row_bytes, num_rows);
        UpdateTexture(texture, 0, 0, width, height, data, row_bytes, num_bytes, num_rows);

        stbi_image_free(data);
    }
    return texture;
}

void RenderModel::UpdateBuffer(const std::shared_ptr<Resource>& buffer,
                               uint64_t buffer_offset,
                               const void* data,
                               uint64_t num_bytes)
{
    std::shared_ptr<Resource> upload_buffer =
        device_->CreateBuffer(MemoryType::kUpload, { .size = num_bytes, .usage = BindFlag::kCopySource });
    upload_buffer->UpdateUploadBuffer(0, data, num_bytes);
    BufferCopyRegion copy_region = {
        .src_offset = 0,
        .dst_offset = buffer_offset,
        .num_bytes = num_bytes,
    };
    command_list_->CopyBuffer(upload_buffer, buffer, { copy_region });
    upload_buffers_.push_back(upload_buffer);
}

void RenderModel::UpdateTexture(const std::shared_ptr<Resource>& texture,
                                uint32_t mip_level,
                                uint32_t array_layer,
                                uint32_t width,
                                uint32_t height,
                                const void* data,
                                uint64_t row_pitch,
                                uint64_t slice_pitch,
                                uint32_t num_rows)
{
    uint64_t aligned_row_pitch = Align(row_pitch, device_->GetTextureDataPitchAlignment());
    uint64_t buffer_size = aligned_row_pitch * num_rows;
    std::shared_ptr<Resource> upload_buffer =
        device_->CreateBuffer(MemoryType::kUpload, { .size = buffer_size, .usage = BindFlag::kCopySource });
    BufferToTextureCopyRegion copy_region = {
        .buffer_offset = 0,
        .buffer_row_pitch = static_cast<uint32_t>(aligned_row_pitch),
        .texture_mip_level = mip_level,
        .texture_array_layer = array_layer,
        .texture_extent = { .width = width, .height = height, .depth = 1 },
    };
    upload_buffer->UpdateUploadBufferWithTextureData(copy_region.buffer_offset, copy_region.buffer_row_pitch,
                                                     buffer_size, data, row_pitch, slice_pitch, row_pitch, num_rows, 1);
    command_list_->CopyBufferToTexture(upload_buffer, texture, { copy_region });
    upload_buffers_.push_back(upload_buffer);
}
