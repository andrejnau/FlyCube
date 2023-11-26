#include "Shader/MTShader.h"

#include "HLSLCompiler/MSLConverter.h"

static std::string FixEntryPoint(const std::string& entry_point)
{
    if (entry_point == "main") {
        return "main0";
    }
    return entry_point;
}

MTShader::MTShader(const std::vector<uint8_t>& blob, ShaderBlobType blob_type, ShaderType shader_type)
    : ShaderBase(blob, blob_type, shader_type)
{
    m_source = GetMSLShader(m_blob, m_index_mapping);
}

MTShader::MTShader(const ShaderDesc& desc, ShaderBlobType blob_type)
    : ShaderBase(desc, blob_type)
{
    m_source = GetMSLShader(m_blob, m_index_mapping);
}

const std::string& MTShader::GetSource() const
{
    return m_source;
}

uint32_t MTShader::GetIndex(BindKey bind_key) const
{
    return m_index_mapping.at(m_bindings.at(m_mapping.at(bind_key)).name);
}

id<MTLLibrary> MTShader::CreateLibrary(id<MTLDevice> device)
{
    NSString* ns_source = [NSString stringWithUTF8String:m_source.c_str()];
    NSError* error = nullptr;
    id<MTLLibrary> library = [device newLibraryWithSource:ns_source options:nullptr error:&error];
    if (library == nullptr) {
        NSLog(@"Error: failed to create Metal library: %@", error);
    }
    return library;
}

id<MTLFunction> MTShader::CreateFunction(id<MTLLibrary> library, const std::string& entry_point)
{
    NSString* ns_entry_point = [NSString stringWithUTF8String:FixEntryPoint(entry_point).c_str()];
    MTLFunctionConstantValues* constant_values = [MTLFunctionConstantValues new];
    NSError* error = nullptr;
    id<MTLFunction> function = [library newFunctionWithName:ns_entry_point constantValues:constant_values error:&error];
    if (function == nullptr) {
        NSLog(@"Error: failed to create Metal function: %@", error);
    }
    return function;
}
