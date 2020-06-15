#include <Utilities/FileUtility.h>
#include <Utilities/DXUtility.h>
#include <HLSLCompiler/DXCompiler.h>
#include <HLSLCompiler/DXReflector.h>
#include <mustache.hpp>
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <fstream>
#include <iterator>
#include <cctype>
#include <map>
#include <set>
using namespace kainjow;

struct Option
{
    std::string shader_name;
    std::string shader_path;
    std::string entrypoint;
    std::string type;
    std::string model;
    std::string template_path;
    std::string output_dir;
};

inline void ThrowIfFailed(bool res, const std::string& msg)
{
    if (!res)
        throw std::runtime_error(msg);
}

class ShaderReflection
{
public:
    ShaderReflection(const Option& option)
        : m_option(option)
        , m_entrypoint(m_option.entrypoint)
        , m_tmpl(ReadFile(m_option.template_path))
    {
        if (m_option.type == "Library")
        {
            m_target = "lib_" + m_option.model;
            m_target.replace(m_target.find("."), 1, "_");
            m_entrypoint = {};
        }
        else
        {
            m_target = "xs_" + m_option.model;
            m_target.replace(m_target.find("."), 1, "_");
            m_target.front() = std::tolower(m_option.type[0]);
        }

        m_type = GetShaderType(m_target);
    }

    ShaderType GetShaderType(const std::string& target)
    {
        if (target.find("ps") == 0)
            return ShaderType::kPixel;
        else if (target.find("vs") == 0)
            return ShaderType::kVertex;
        else if (target.find("cs") == 0)
            return ShaderType::kCompute;
        else if (target.find("gs") == 0)
            return ShaderType::kGeometry;
        else if (target.find("lib") == 0)
            return ShaderType::kLibrary;
        assert(false);
        return ShaderType::kUnknown;
    }

    void Parse()
    {
        auto blob = DXCompile({ m_option.shader_path, m_entrypoint, m_type });

        ComPtr<ID3D12ShaderReflection> shader_reflector;
        DXReflect(blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&shader_reflector));
        if (shader_reflector)
            ParseShader(shader_reflector);
        else
        {
            ComPtr<ID3D12LibraryReflection> library_reflector;
            DXReflect(blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&library_reflector));
            if (library_reflector)
                ParseLibrary(library_reflector);
        }
    }

    void Gen()
    {
        std::ifstream is(m_option.output_dir + "/" + m_option.shader_name + ".h");
        std::string old_content((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
        std::stringstream buf;
        m_tmpl.render(m_tdevice, buf);
        std::string new_content = buf.str();
        if (new_content != old_content)
        {
            std::ofstream os(m_option.output_dir + "/" + m_option.shader_name + ".h");
            os << new_content;
        }
    }

private:
    std::string ReadFile(const std::string& path)
    {
        std::ifstream is(path);
        return std::string(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
    }

    std::string TargetToShaderType(const std::string& str)
    {
        if (str.find("ps") == 0)
            return "ShaderType::kPixel";
        else if (str.find("vs") == 0)
            return "ShaderType::kVertex";
        else if (str.find("cs") == 0)
            return "ShaderType::kCompute";
        else if (str.find("gs") == 0)
            return "ShaderType::kGeometry";
        else if (str.find("lib") == 0)
            return "ShaderType::kLibrary";
        return "";
    }

    std::string TargetToShaderPrefix(const std::string& str)
    {
        if (str.find("ps") == 0)
            return "PS";
        else if (str.find("vs") == 0)
            return "VS";
        else if (str.find("cs") == 0)
            return "CS";
        else if (str.find("gs") == 0)
            return "GS";
        return "";
    }

    std::string TypeFromDesc(const D3D12_SHADER_TYPE_DESC& desc)
    {
        std::string type_prefix;
        std::string type;
        std::string res;
        switch (desc.Type)
        {
        case D3D_SHADER_VARIABLE_TYPE::D3D_SVT_FLOAT:
            type_prefix = "";
            type = "float";
            break;
        case D3D_SHADER_VARIABLE_TYPE::D3D_SVT_INT:
            type_prefix = "i";
            type = "int32_t";
            break;
        case D3D_SHADER_VARIABLE_TYPE::D3D_SVT_UINT:
            type_prefix = "u";
            type = "uint32_t";
            break;
        case D3D_SHADER_VARIABLE_TYPE::D3D_SVT_BOOL:
            type_prefix = "b";
            type = "uint32_t";
            break;
        }

        switch (desc.Class)
        {
        case D3D_SHADER_VARIABLE_CLASS::D3D_SVC_SCALAR:
            res = type;
            break;
        case D3D_SHADER_VARIABLE_CLASS::D3D_SVC_VECTOR:
            res = "glm::" + type_prefix + "vec" + std::to_string(desc.Columns);
            break;
        case D3D_SHADER_VARIABLE_CLASS::D3D_SVC_MATRIX_COLUMNS:
            res = "glm::" + type_prefix + "mat" + std::to_string(desc.Rows) + "x" + std::to_string(desc.Columns);
            break;
        }

        if (desc.Elements > 0)
            res = "std::array<" + res + ", " + std::to_string(desc.Elements) + ">";

        return res;
    }

    void ParseShader(ComPtr<ID3D12ShaderReflection>& reflector)
    {
        m_tdevice["ShaderName"] = m_option.shader_name;
        m_tdevice["ShaderType"] = TargetToShaderType(m_target);
        m_tdevice["ShaderPrefix"] = TargetToShaderPrefix(m_target);
        m_tdevice["ShaderPath"] = m_option.shader_path;
        m_tdevice["Entrypoint"] = m_entrypoint;
        m_tdevice["Target"] = m_target;

        mustache::data tcbuffers{ mustache::data::type::list };

        D3D12_SHADER_DESC desc = {};
        reflector->GetDesc(&desc);

        for (UINT i = 0; i < desc.ConstantBuffers; ++i)
        {
            ID3D12ShaderReflectionConstantBuffer* cbuffer = reflector->GetConstantBufferByIndex(i);
            D3D12_SHADER_BUFFER_DESC cbdesc = {};
            cbuffer->GetDesc(&cbdesc);

            if (cbdesc.Type != D3D_CT_CBUFFER)
                continue;

            D3D12_SHADER_INPUT_BIND_DESC res_desc = {};
            ASSERT_SUCCEEDED(reflector->GetResourceBindingDescByName(cbdesc.Name, &res_desc));

            mustache::data tcbuffer;
            tcbuffer.set("Name", cbdesc.Name);
            tcbuffer.set("Slot", std::to_string(res_desc.BindPoint));
            tcbuffer.set("Space", std::to_string(res_desc.Space));
            tcbuffer.set("BufferName", cbdesc.Name);
            tcbuffer.set("BufferSize", std::to_string(cbdesc.Size));
            tcbuffer.set("BufferSeparator", tcbuffers.is_empty_list() ? ":" : ",");

            mustache::data tvariables{ mustache::data::type::list };

            for (UINT i = 0; i < cbdesc.Variables; ++i)
            {
                ID3D12ShaderReflectionVariable* variable = cbuffer->GetVariableByIndex(i);

                D3D12_SHADER_VARIABLE_DESC vdesc;
                variable->GetDesc(&vdesc);

                ID3D12ShaderReflectionType* vtype = variable->GetType();

                D3D12_SHADER_TYPE_DESC type_desc = {};
                vtype->GetDesc(&type_desc);

                mustache::data tvariable;
                tvariable.set("Name", vdesc.Name);
                tvariable.set("StartOffset", std::to_string(vdesc.StartOffset));
                tvariable.set("VariableSize", std::to_string(vdesc.Size));
                tvariable.set("Type", TypeFromDesc(type_desc));
                
                tvariables.push_back(tvariable);
            }
            tcbuffer.set("Variables", tvariables);

            tcbuffers.push_back(tcbuffer);
        }
        m_tdevice["CBuffers"] = mustache::data{ tcbuffers };
        m_tdevice["HasCBuffers"] = tcbuffers.is_non_empty_list();

        mustache::data ttextures{ mustache::data::type::list };
        mustache::data tuavs{ mustache::data::type::list };
        mustache::data tsamplers{ mustache::data::type::list };
        for (UINT i = 0; i < desc.BoundResources; ++i)
        {
            D3D12_SHADER_INPUT_BIND_DESC res_desc = {};
            ASSERT_SUCCEEDED(reflector->GetResourceBindingDesc(i, &res_desc));
            switch (res_desc.Type)
            {
            case D3D_SIT_SAMPLER:
            {
                mustache::data tsampler;
                tsampler.set("Name", res_desc.Name);
                tsampler.set("Slot", std::to_string(res_desc.BindPoint));
                tsampler.set("Space", std::to_string(res_desc.Space));
                tsampler.set("Separator", tsamplers.is_empty_list() ? ":" : ",");
                tsamplers.push_back(tsampler);
                break;
                break;
            }
            case D3D_SIT_TEXTURE:
            case D3D_SIT_STRUCTURED:
            {
                mustache::data ttexture;
                ttexture.set("Name", res_desc.Name);
                ttexture.set("Slot", std::to_string(res_desc.BindPoint));
                ttexture.set("Space", std::to_string(res_desc.Space));
                ttexture.set("Separator", ttextures.is_empty_list() ? ":" : ",");
                ttextures.push_back(ttexture);
                break;  
            }
            case D3D_SIT_UAV_RWSTRUCTURED:
            case D3D_SIT_UAV_RWTYPED:
            case D3D_SIT_UAV_RWBYTEADDRESS:
            case D3D_SIT_UAV_APPEND_STRUCTURED:
            case D3D_SIT_UAV_CONSUME_STRUCTURED:
            case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
            {
                mustache::data tuav;
                tuav.set("Name", res_desc.Name);
                tuav.set("Slot", std::to_string(res_desc.BindPoint));
                tuav.set("Space", std::to_string(res_desc.Space));
                tuav.set("Separator", tuavs.is_empty_list() ? ":" : ",");
                tuavs.push_back(tuav);
                break;
            }
            }
        }
        m_tdevice["Textures"] = mustache::data{ ttextures };
        m_tdevice["HasTextures"] = ttextures.is_non_empty_list();
        m_tdevice["UAVs"] = mustache::data{ tuavs };
        m_tdevice["HasUAVs"] = tuavs.is_non_empty_list();
        m_tdevice["Samplers"] = mustache::data{ tsamplers };
        m_tdevice["HasSamplers"] = tsamplers.is_non_empty_list();

        if (m_target.find("vs") != -1)
        {
            mustache::data tinputs{ mustache::data::type::list };
            std::map<std::string, size_t> use_name;
            for (UINT i = 0; i < desc.InputParameters; ++i)
            {
                D3D12_SIGNATURE_PARAMETER_DESC param_desc = {};
                reflector->GetInputParameterDesc(i, &param_desc);
                ++use_name[param_desc.SemanticName];
            }
            for (UINT i = 0; i < desc.InputParameters; ++i)
            {
                D3D12_SIGNATURE_PARAMETER_DESC param_desc = {};
                reflector->GetInputParameterDesc(i, &param_desc);

                mustache::data tinput;
                std::string input_name = param_desc.SemanticName;
                if (param_desc.SemanticIndex || use_name[param_desc.SemanticName] > 1)
                    input_name += std::to_string(param_desc.SemanticIndex);
                tinput.set("Name", input_name);
                tinput.set("Slot", std::to_string(i));
                tinputs.push_back(tinput);
            }
            m_tdevice["Inputs"] = mustache::data{ tinputs };
            m_tdevice["HasInputs"] = tinputs.is_non_empty_list();
        }

        if (m_target.find("ps") != -1)
        {
            mustache::data toutputs{ mustache::data::type::list };

            for (UINT i = 0; i < desc.OutputParameters; ++i)
            {
                D3D12_SIGNATURE_PARAMETER_DESC param_desc = {};
                reflector->GetOutputParameterDesc(i, &param_desc);

                mustache::data toutput;
                std::string input_name = param_desc.SemanticName;
                if (param_desc.SemanticIndex || desc.OutputParameters > 1)
                    input_name += std::to_string(param_desc.SemanticIndex);
                toutput.set("Name", input_name);
                toutput.set("Slot", std::to_string(i));
                toutput.set("Separator", toutputs.is_empty_list() ? ":" : ",");
                toutputs.push_back(toutput);
            }
            m_tdevice["Outputs"] = mustache::data{ toutputs };
            m_tdevice["DSVSeparator"] = toutputs.is_empty_list() ? ":" : ",";
            m_tdevice["HasOutputs"] = true;
        }
    }

    void ParseLibrary(ComPtr<ID3D12LibraryReflection>& library_reflector)
    {
        m_tdevice["ShaderName"] = m_option.shader_name;
        m_tdevice["ShaderType"] = TargetToShaderType(m_target);
        m_tdevice["ShaderPrefix"] = TargetToShaderPrefix(m_target);
        m_tdevice["ShaderPath"] = m_option.shader_path;
        m_tdevice["Entrypoint"] = m_entrypoint;
        m_tdevice["Target"] = m_target;

        mustache::data tcbuffers{ mustache::data::type::list };
        std::set<std::string> resources;
        D3D12_LIBRARY_DESC lib_desc = {};
        library_reflector->GetDesc(&lib_desc);
        for (UINT j = 0; j < lib_desc.FunctionCount; ++j)
        {
            auto reflector = library_reflector->GetFunctionByIndex(j);
            D3D12_FUNCTION_DESC desc = {};
            reflector->GetDesc(&desc);
            for (UINT i = 0; i < desc.ConstantBuffers; ++i)
            {
                ID3D12ShaderReflectionConstantBuffer* cbuffer = reflector->GetConstantBufferByIndex(i);
                D3D12_SHADER_BUFFER_DESC cbdesc = {};
                cbuffer->GetDesc(&cbdesc);

                if (cbdesc.Type != D3D_CT_CBUFFER)
                    continue;

                if (resources.count(cbdesc.Name))
                    continue;
                resources.insert(cbdesc.Name);

                D3D12_SHADER_INPUT_BIND_DESC res_desc = {};
                ASSERT_SUCCEEDED(reflector->GetResourceBindingDescByName(cbdesc.Name, &res_desc));

                mustache::data tcbuffer;
                tcbuffer.set("Name", cbdesc.Name);
                tcbuffer.set("BufferName", cbdesc.Name);
                tcbuffer.set("BufferSize", std::to_string(cbdesc.Size));
                tcbuffer.set("Slot", std::to_string(res_desc.BindPoint));
                tcbuffer.set("Space", std::to_string(res_desc.Space));
                tcbuffer.set("BufferSeparator", tcbuffers.is_empty_list() ? ":" : ",");

                mustache::data tvariables{ mustache::data::type::list };

                for (UINT i = 0; i < cbdesc.Variables; ++i)
                {
                    ID3D12ShaderReflectionVariable* variable = cbuffer->GetVariableByIndex(i);

                    D3D12_SHADER_VARIABLE_DESC vdesc;
                    variable->GetDesc(&vdesc);

                    ID3D12ShaderReflectionType* vtype = variable->GetType();

                    D3D12_SHADER_TYPE_DESC type_desc = {};
                    vtype->GetDesc(&type_desc);

                    mustache::data tvariable;
                    tvariable.set("Name", vdesc.Name);
                    tvariable.set("StartOffset", std::to_string(vdesc.StartOffset));
                    tvariable.set("VariableSize", std::to_string(vdesc.Size));
                    tvariable.set("Type", TypeFromDesc(type_desc));

                    tvariables.push_back(tvariable);
                }
                tcbuffer.set("Variables", tvariables);

                tcbuffers.push_back(tcbuffer);
            }
        }
        m_tdevice["CBuffers"] = mustache::data{ tcbuffers };
        m_tdevice["HasCBuffers"] = tcbuffers.is_non_empty_list();

        mustache::data ttextures{ mustache::data::type::list };
        mustache::data tuavs{ mustache::data::type::list };
        mustache::data tsamplers{ mustache::data::type::list };

        for (UINT j = 0; j < lib_desc.FunctionCount; ++j)
        {
            auto reflector = library_reflector->GetFunctionByIndex(j);
            D3D12_FUNCTION_DESC desc = {};
            reflector->GetDesc(&desc);
            for (UINT i = 0; i < desc.BoundResources; ++i)
            {
                D3D12_SHADER_INPUT_BIND_DESC res_desc = {};
                ASSERT_SUCCEEDED(reflector->GetResourceBindingDesc(i, &res_desc));
                if (resources.count(res_desc.Name))
                    continue;
                resources.insert(res_desc.Name);
                switch (res_desc.Type)
                {
                case D3D_SIT_SAMPLER:
                {
                    mustache::data tsampler;
                    tsampler.set("Name", res_desc.Name);
                    tsampler.set("Slot", std::to_string(res_desc.BindPoint));
                    tsampler.set("Space", std::to_string(res_desc.Space));
                    tsampler.set("Separator", tsamplers.is_empty_list() ? ":" : ",");
                    tsamplers.push_back(tsampler);
                    break;
                    break;
                }
                case D3D_SIT_TEXTURE:
                case D3D_SIT_STRUCTURED:
                case SIT_RTACCELERATIONSTRUCTURE:
                {
                    mustache::data ttexture;
                    ttexture.set("Name", res_desc.Name);
                    ttexture.set("Slot", std::to_string(res_desc.BindPoint));
                    ttexture.set("Space", std::to_string(res_desc.Space));
                    ttexture.set("Separator", ttextures.is_empty_list() ? ":" : ",");
                    ttextures.push_back(ttexture);
                    break;
                }
                case D3D_SIT_UAV_RWSTRUCTURED:
                case D3D_SIT_UAV_RWTYPED:
                case D3D_SIT_UAV_RWBYTEADDRESS:
                case D3D_SIT_UAV_APPEND_STRUCTURED:
                case D3D_SIT_UAV_CONSUME_STRUCTURED:
                case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                {
                    mustache::data tuav;
                    tuav.set("Name", res_desc.Name);
                    tuav.set("Slot", std::to_string(res_desc.BindPoint));
                    tuav.set("Space", std::to_string(res_desc.Space));
                    tuav.set("Separator", tuavs.is_empty_list() ? ":" : ",");
                    tuavs.push_back(tuav);
                    break;
                }
                }
            }
        }
        m_tdevice["Textures"] = mustache::data{ ttextures };
        m_tdevice["HasTextures"] = ttextures.is_non_empty_list();
        m_tdevice["UAVs"] = mustache::data{ tuavs };
        m_tdevice["HasUAVs"] = tuavs.is_non_empty_list();
        m_tdevice["Samplers"] = mustache::data{ tsamplers };
        m_tdevice["HasSamplers"] = tsamplers.is_non_empty_list();
    }

    const Option& m_option;
    mustache::mustache m_tmpl;
    mustache::data m_tdevice;
    std::string m_target;
    ShaderType m_type;
    std::string m_entrypoint;
};

class ParseCmd
{
public:
    ParseCmd(int argc, char *argv[])
    {
        ThrowIfFailed(argc == 8, "Invalide CommandLine");
        size_t arg_index = 1;
        m_option.shader_name = argv[arg_index++];
        m_option.shader_path = argv[arg_index++];
        m_option.entrypoint = argv[arg_index++];
        m_option.type = argv[arg_index++];
        m_option.model = argv[arg_index++];
        m_option.template_path = argv[arg_index++];
        m_option.output_dir = argv[arg_index++];
    }

    const Option& GetOption() const
    {
        return m_option;
    }

    static void Usage()
    {
        std::cout << "TODO" << std::endl;
    }

private:
    Option m_option;
};

int main(int argc, char *argv[])
{
    try
    {
        ParseCmd cmd(argc, argv);
        ShaderReflection ref(cmd.GetOption());
        ref.Parse();
        ref.Gen();
    }
    catch (std::exception&)
    {
        ParseCmd::Usage();
        return ~0;
    }
    return 0;
}
