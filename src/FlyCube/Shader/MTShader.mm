#include "Shader/MTShader.h"

#include "Device/MTDevice.h"
#include "Utilities/Logging.h"

namespace {

std::string FixEntryPoint(const std::string& entry_point)
{
    if (entry_point == "main") {
        return "main0";
    }
    return entry_point;
}

} // namespace

MTShader::MTShader(MTDevice& device, const std::vector<uint8_t>& blob, ShaderBlobType blob_type, ShaderType shader_type)
    : ShaderBase(blob, blob_type, shader_type, /*is_msl*/ true)
    , m_device(device)
{
    CreateLibrary();
}

MTShader::MTShader(MTDevice& device, const ShaderDesc& desc, ShaderBlobType blob_type)
    : ShaderBase(desc, blob_type, /*is_msl*/ true)
    , m_device(device)
{
    CreateLibrary();
}

const std::string& MTShader::GetSource() const
{
    return m_msl_source;
}

uint32_t MTShader::GetIndex(BindKey bind_key) const
{
    return m_slot_remapping.at(bind_key);
}

MTL4LibraryFunctionDescriptor* MTShader::CreateFunctionDescriptor(const std::string& entry_point)
{
    MTL4LibraryFunctionDescriptor* function_descriptor = [MTL4LibraryFunctionDescriptor new];
    function_descriptor.library = m_library;
    function_descriptor.name = [NSString stringWithUTF8String:FixEntryPoint(entry_point).c_str()];
    return function_descriptor;
}

void MTShader::CreateLibrary()
{
    MTL4LibraryDescriptor* library_descriptor = [MTL4LibraryDescriptor new];
    library_descriptor.source = [NSString stringWithUTF8String:m_msl_source.c_str()];
    NSError* error = nullptr;
    m_library = [m_device.GetCompiler() newLibraryWithDescriptor:library_descriptor error:&error];
    if (!m_library) {
        Logging::Println("Failed to create MTLLibrary: {}", error);
    }
}
