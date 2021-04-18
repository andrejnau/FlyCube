#include "ShaderParser.h"
#include <string>
#include <sstream>
#include <HLSLCompiler/Compiler.h>
#include <ShaderReflection/ShaderReflection.h>
#include <mustache.hpp>
using namespace kainjow;

class ShaderParser
{
public:
    ShaderParser(const Option& option)
        : m_option(option)
        , m_template(m_option.mustache_template)
    {
        auto blob = Compile({ m_option.assets_path + m_option.shader_path, m_option.entrypoint, m_option.type, m_option.model }, m_option.blob_type);
        auto reflection = CreateShaderReflection(m_option.blob_type, blob.data(), blob.size());
        ParseReflection(reflection);
    }

    std::string Render()
    {
        std::stringstream buffer;
        m_template.render(m_data, buffer);
        return buffer.str();
    }

private:
    std::string ShaderTypeToString(ShaderType type)
    {
        switch (type)
        {
        case ShaderType::kVertex:
            return "ShaderType::kVertex";
        case ShaderType::kPixel:
            return "ShaderType::kPixel";
        case ShaderType::kCompute:
            return "ShaderType::kCompute";
        case ShaderType::kGeometry:
            return "ShaderType::kGeometry";
        case ShaderType::kAmplification:
            return "ShaderType::kAmplification";
        case ShaderType::kMesh:
            return "ShaderType::kMesh";
        case ShaderType::kLibrary:
            return "ShaderType::kLibrary";
        default:
            assert(false);
            return "ShaderType::kUnknown";
        }
    }

    std::string GenerateCppType(const VariableLayout& desc)
    {
        std::string glm_prefix;
        std::string base_type;
        switch (desc.type)
        {
        case VariableType::kFloat:
            glm_prefix = "";
            base_type = "float";
            break;
        case VariableType::kInt:
            glm_prefix = "i";
            base_type = "int32_t";
            break;
        case VariableType::kUint:

            glm_prefix = "u";
            base_type = "uint32_t";
            break;
        case VariableType::kBool:
            glm_prefix = "b";
            base_type = "uint32_t";
            break;
        default:
            assert(false);
            return {};
        }

        std::string type;
        if (desc.rows > 1 && desc.columns > 1)
        {
            type = "glm::" + glm_prefix + "mat" + std::to_string(desc.columns) + "x" + std::to_string(desc.rows);
        }
        else if (desc.columns > 1)
        {
            type = "glm::" + glm_prefix + "vec" + std::to_string(desc.columns);
        }
        else
        {
            type = base_type;
        }

        if (desc.elements > 1)
        {
            return "std::array<" + type + ", " + std::to_string(desc.elements) + ">";
        }
        else
        {
            return type;
        }
    }

    mustache::data GetVariables(const VariableLayout& layout)
    {
        assert(layout.type == VariableType::kStruct);
        mustache::data variables = { mustache::data::type::list };
        for (const auto& member : layout.members)
        {
            mustache::data variable = {};
            variable.set("VariableName", member.name);
            variable.set("VariableOffset", std::to_string(member.offset));
            variable.set("VariableSize", std::to_string(member.size));
            variable.set("VariableType", GenerateCppType(member));
            variables.push_back(variable);
        }
        return variables;
    }

    void ParseReflection(const std::shared_ptr<ShaderReflection>& reflection)
    {
        mustache::data cbuffers = { mustache::data::type::list };
        mustache::data textures = { mustache::data::type::list };
        mustache::data uavs = { mustache::data::type::list };
        mustache::data samplers = { mustache::data::type::list };
        mustache::data inputs = { mustache::data::type::list };
        mustache::data outputs = { mustache::data::type::list };

        auto& bindings = reflection->GetBindings();
        auto& layouts = reflection->GetVariableLayouts();
        for (size_t i = 0; i < bindings.size(); ++i)
        {
            mustache::data binding_desc = {};
            binding_desc.set("Name", bindings[i].name);
            binding_desc.set("Slot", std::to_string(bindings[i].slot));
            binding_desc.set("Space", std::to_string(bindings[i].space));

            switch (bindings[i].type)
            {
            case ViewType::kConstantBuffer:
            {
                assert(layouts[i].offset == 0);
                binding_desc.set("CBufferSize", std::to_string(layouts[i].size));
                binding_desc.set("CBufferSeparator", cbuffers.is_empty_list() ? ":" : ",");
                binding_desc.set("CBufferVariables", GetVariables(layouts[i]));
                cbuffers.push_back(binding_desc);
                break;
            }
            case ViewType::kSampler:
            {
                samplers.push_back(binding_desc);
                break;
            }
            case ViewType::kTexture:
            case ViewType::kBuffer:
            case ViewType::kStructuredBuffer:
            case ViewType::kAccelerationStructure:
            {
                textures.push_back(binding_desc);
                break;
            }
            case ViewType::kRWTexture:
            case ViewType::kRWBuffer:
            case ViewType::kRWStructuredBuffer:
            {
                uavs.push_back(binding_desc);
                break;
            }
            }
        }

        if (m_option.type == ShaderType::kVertex)
        {
            auto& input_parameters = reflection->GetInputParameters();
            for (size_t i = 0; i < input_parameters.size(); ++i)
            {
                mustache::data input_desc = {};
                std::string input_name = input_parameters[i].semantic_name;
                input_desc.set("Name", input_parameters[i].semantic_name);
                input_desc.set("Slot", std::to_string(i));
                inputs.push_back(input_desc);
            }
        }
        else if (m_option.type == ShaderType::kPixel)
        {
            auto& output_parameters = reflection->GetOutputParameters();
            for (size_t i = 0; i < output_parameters.size(); ++i)
            {
                mustache::data output_desc = {};
                output_desc.set("Slot", std::to_string(output_parameters[i].slot));
                outputs.push_back(output_desc);
            }
        }

        m_data["ShaderName"] = m_option.shader_name;
        m_data["ShaderPath"] = m_option.shader_path;
        m_data["ShaderType"] = ShaderTypeToString(m_option.type);
        m_data["ShaderModel"] = m_option.model;
        m_data["Entrypoint"] = m_option.entrypoint;
        m_data["CBuffers"] = mustache::data{ cbuffers };
        m_data["HasCBuffers"] = cbuffers.is_non_empty_list();
        m_data["Textures"] = mustache::data{ textures };
        m_data["HasTextures"] = textures.is_non_empty_list();
        m_data["UAVs"] = mustache::data{ uavs };
        m_data["HasUAVs"] = uavs.is_non_empty_list();
        m_data["Samplers"] = mustache::data{ samplers };
        m_data["HasSamplers"] = samplers.is_non_empty_list();
        m_data["Inputs"] = mustache::data{ inputs };
        m_data["HasInputs"] = inputs.is_non_empty_list();
        m_data["Outputs"] = mustache::data{ outputs };
        m_data["HasOutputs"] = outputs.is_non_empty_list();
    }

    const Option& m_option;
    mustache::mustache m_template;
    mustache::data m_data;
};

std::string RenderShaderReflection(const Option& option)
{
    ShaderParser parser(option);
    return parser.Render();
}
