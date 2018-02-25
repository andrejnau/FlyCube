#pragma once

#include <Context/DX11Context.h>
#include "Program/ProgramApi.h"
#include "Program/ShaderApi.h"
#include "Program/DX11VertexShaderApi.h"
#include "Program/DX11GeometryShaderApi.h"
#include "Program/DX11PixelShaderApi.h"
#include "Program/DX11ComputeShaderApi.h"

class DX11ProgramApi : public ProgramApi
{
public:
    DX11ProgramApi(DX11Context& context)
        : m_context(context)
    {
    }

    virtual std::unique_ptr<ShaderApi> CreateShader(ShaderType type) override
    {
        switch (type)
        {
        case ShaderType::kVertex:
            return std::make_unique<DX11VertexShaderApi>(m_context);
        case ShaderType::kPixel:
            return std::make_unique<DX11PixelShaderApi>(m_context);
        case ShaderType::kCompute:
            return std::make_unique<DX11ComputeShaderApi>(m_context);
        case ShaderType::kGeometry:
            return std::make_unique<DX11GeometryShaderApi>(m_context);
        }
        return nullptr;
    }

    virtual void UseProgram() override
    {
        m_context.device_context->VSSetShader(nullptr, nullptr, 0);
        m_context.device_context->GSSetShader(nullptr, nullptr, 0);
        m_context.device_context->DSSetShader(nullptr, nullptr, 0);
        m_context.device_context->HSSetShader(nullptr, nullptr, 0);
        m_context.device_context->PSSetShader(nullptr, nullptr, 0);
        m_context.device_context->CSSetShader(nullptr, nullptr, 0);
    }
private:
    DX11Context& m_context;
};
