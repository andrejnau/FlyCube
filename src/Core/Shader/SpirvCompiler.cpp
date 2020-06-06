#include "Shader/SpirvCompiler.h"
#include <iostream>
#include <fstream>
#include <Utilities/FileUtility.h>
#include <cassert>

#include "Shader/DXCompiler.h"

std::vector<uint32_t> SpirvCompile(const ShaderDesc& shader, const SpirvOption& option)
{
    DXOption dx_option = { true, option.invert_y };
    auto blob = DXCompile(shader, dx_option);
    if (!blob)
        return {};
    auto blob_as_uint32 = reinterpret_cast<uint32_t*>(blob->GetBufferPointer());
    std::vector<uint32_t> spirv(blob_as_uint32, blob_as_uint32 + blob->GetBufferSize() / 4);
    return spirv;
}
