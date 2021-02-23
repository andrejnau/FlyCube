#pragma once
#include "ShaderReflection/ShaderReflection.h"
#include <vector>
#include <string>

class SPIRVReflection : public ShaderReflection
{
public:
    SPIRVReflection(const void* data, size_t size);
    const std::vector<EntryPoint> GetEntryPoints() const override;

private:
    std::vector<uint32_t> m_blob;
    std::vector<EntryPoint> m_entry_points;
};
