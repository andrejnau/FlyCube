#include "HLSLCompiler/SpirvCompiler.h"
#include "HLSLCompiler/DXCompiler.h"
#include <iostream>
#include <fstream>
#include <Utilities/FileUtility.h>
#include <cassert>

std::vector<uint32_t> SpirvCompile(const ShaderDesc& shader)
{
    auto blob = DXCompile(shader, { true });
    if (!blob)
        return {};
    auto blob_as_uint32 = reinterpret_cast<uint32_t*>(blob->GetBufferPointer());
    std::vector<uint32_t> spirv(blob_as_uint32, blob_as_uint32 + blob->GetBufferSize() / 4);
    return spirv;
}
