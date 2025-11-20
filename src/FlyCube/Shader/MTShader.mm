#include "Shader/MTShader.h"

#include "Device/MTDevice.h"
#include "HLSLCompiler/MSLConverter.h"
#include "Utilities/Logging.h"

MTShader::MTShader(MTDevice& device, const std::vector<uint8_t>& blob, ShaderBlobType blob_type, ShaderType shader_type)
    : ShaderBase(blob, blob_type, shader_type)
{
    std::string entry_point;
    std::string msl_source = GetMSLShader(shader_type, blob_, slot_remapping_, entry_point);

    MTL4LibraryDescriptor* library_descriptor = [MTL4LibraryDescriptor new];
    library_descriptor.source = [NSString stringWithUTF8String:msl_source.c_str()];
    NSError* error = nullptr;
    id<MTLLibrary> library = [device.GetCompiler() newLibraryWithDescriptor:library_descriptor error:&error];
    if (!library) {
        Logging::Println("Failed to create MTLLibrary: {}", error);
    }

    function_descriptor_ = [MTL4LibraryFunctionDescriptor new];
    function_descriptor_.library = library;
    function_descriptor_.name = [NSString stringWithUTF8String:entry_point.c_str()];
}

uint32_t MTShader::GetIndex(BindKey bind_key) const
{
    return slot_remapping_.at(bind_key);
}

MTL4LibraryFunctionDescriptor* MTShader::GetFunctionDescriptor()
{
    return function_descriptor_;
}
