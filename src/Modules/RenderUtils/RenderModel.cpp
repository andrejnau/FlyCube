#include "RenderUtils/RenderModel.h"

#include "Utilities/Asset.h"
#include "Utilities/Common.h"
#include "Utilities/FormatHelper.h"

#include <stb_image.h>

namespace {

template <typename T>
size_t GetNumBytes(const std::vector<T>& data)
{
    return sizeof(data.front()) * data.size();
}

} // namespace

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

    uint64_t buffer_size = 0;
    for (const auto& mesh : model->meshes) {
        buffer_size += GetNumBytes(mesh.indices);
        buffer_size += GetNumBytes(mesh.positions);
        buffer_size += GetNumBytes(mesh.normals);
        buffer_size += GetNumBytes(mesh.tangents);
        buffer_size += GetNumBytes(mesh.texcoords);
    }
    std::shared_ptr<Resource> upload_buffer =
        CreateUploadBuffer({ .size = buffer_size, .usage = BindFlag::kCopySource });
    std::shared_ptr<Resource> buffer = device_->CreateBuffer(
        MemoryType::kDefault,
        { .size = buffer_size, .usage = BindFlag::kCopyDest | BindFlag::kIndexBuffer | BindFlag::kVertexBuffer });

    size_t buffer_offset = 0;
    auto get_buffer = [&](const auto& data) -> RenderMesh::Buffer {
        if (data.empty()) {
            return {};
        }
        size_t num_bytes = GetNumBytes(data);
        upload_buffer->UpdateUploadBuffer(buffer_offset, data.data(), num_bytes);
        buffer_offset += num_bytes;
        return { buffer, buffer_offset - num_bytes };
    };

    for (size_t i = 0; i < model->meshes.size(); ++i) {
        meshes_[i].matrix = model->meshes[i].matrix;
        meshes_[i].index_count = model->meshes[i].indices.size();
        meshes_[i].index_format = gli::format::FORMAT_R32_UINT_PACK32;

        meshes_[i].indices = get_buffer(model->meshes[i].indices);
        meshes_[i].positions = get_buffer(model->meshes[i].positions);
        meshes_[i].normals = get_buffer(model->meshes[i].normals);
        meshes_[i].tangents = get_buffer(model->meshes[i].tangents);
        meshes_[i].texcoords = get_buffer(model->meshes[i].texcoords);
    }

    BufferCopyRegion copy_region = {
        .src_offset = 0,
        .dst_offset = 0,
        .num_bytes = buffer_size,
    };
    command_list_->CopyBuffer(upload_buffer, buffer, { copy_region });

    for (size_t i = 0; i < model->meshes.size(); ++i) {
        meshes_[i].textures.base_color = CreateTextureFromFile(model->meshes[i].textures.base_color);
        meshes_[i].textures.normal = CreateTextureFromFile(model->meshes[i].textures.normal);
        meshes_[i].textures.metallic_roughness = CreateTextureFromFile(model->meshes[i].textures.metallic_roughness);
        meshes_[i].textures.ambient_occlusion = CreateTextureFromFile(model->meshes[i].textures.ambient_occlusion);
        meshes_[i].textures.emissive = CreateTextureFromFile(model->meshes[i].textures.emissive);
    }

    command_list_->Close();
    command_queue->ExecuteCommandLists({ command_list_ });
    command_queue->Signal(fence_, ++fence_value_);
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

std::shared_ptr<Resource> RenderModel::CreateUploadBuffer(const BufferDesc& desc)
{
    std::shared_ptr<Resource> upload_buffer = device_->CreateBuffer(MemoryType::kUpload, desc);
    upload_buffers_.push_back(upload_buffer);
    return upload_buffer;
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
        CreateUploadBuffer({ .size = buffer_size, .usage = BindFlag::kCopySource });
    BufferTextureCopyRegion copy_region = {
        .buffer_offset = 0,
        .buffer_row_pitch = static_cast<uint32_t>(aligned_row_pitch),
        .texture_mip_level = mip_level,
        .texture_array_layer = array_layer,
        .texture_extent = { .width = width, .height = height, .depth = 1 },
    };
    upload_buffer->UpdateUploadBufferWithTextureData(copy_region.buffer_offset, copy_region.buffer_row_pitch,
                                                     buffer_size, data, row_pitch, slice_pitch, row_pitch, num_rows, 1);
    command_list_->ResourceBarrier({ { texture, ResourceState::kCommon, ResourceState::kCopyDest } });
    command_list_->CopyBufferToTexture(upload_buffer, texture, { copy_region });
}
