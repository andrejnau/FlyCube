#include "Shader/MTShader.h"

#include "Device/MTDevice.h"

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
    return m_slot_remapping.at(m_bindings.at(m_mapping.at(bind_key)).name);
}

id<MTLLibrary> MTShader::GetLibrary() const
{
    return m_library;
}

id<MTLFunction> MTShader::CreateFunction(id<MTLLibrary> library, const std::string& entry_point)
{
    MTLFunctionDescriptor* desc = [MTLFunctionDescriptor functionDescriptor];
    desc.name = [NSString stringWithUTF8String:FixEntryPoint(entry_point).c_str()];
    NSError* error = nullptr;
    id<MTLFunction> function = [library newFunctionWithDescriptor:desc error:&error];
    if (function == nullptr) {
        NSLog(@"Error: failed to create Metal function: %@", error);
    }
    return function;
}

void MTShader::CreateLibrary()
{
    NSString* ns_source = [NSString stringWithUTF8String:m_msl_source.c_str()];
    NSError* error = nullptr;
    m_library = [m_device.GetDevice() newLibraryWithSource:ns_source options:nullptr error:&error];
    if (m_library == nullptr) {
        NSLog(@"Error: failed to create Metal library: %@", error);
    }
}
