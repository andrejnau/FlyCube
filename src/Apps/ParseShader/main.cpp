#include <Utilities/FileUtility.h>
#include <Utilities/DXUtility.h>
#include <mustache.hpp>
#include <d3dcompiler.h>
#include <wrl.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <fstream>
#include <iterator>

using namespace Microsoft::WRL;
using namespace kainjow;

struct Option
{
    std::string shader_name;
    std::string shader_path;
    std::string entrypoint;
    std::string target;
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
        , m_tmpl(ReadFile(m_option.template_path))
    {
    }

    void Parse()
    {
        ParseShader(CompileShader(m_option.shader_path, m_option.entrypoint, m_option.target));
    }

    void Gen()
    {
        std::ofstream os(m_option.output_dir + "/" + m_option.shader_name + ".h");
        m_tmpl.render(m_tcontext, os);
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

    std::string TypeFromDesc(const D3D11_SHADER_TYPE_DESC& desc)
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
            type = "bool";
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

    void ParseShader(ComPtr<ID3DBlob>& shader_buffer)
    {
        m_tcontext["ShaderName"] = m_option.shader_name;
        m_tcontext["ShaderType"] = TargetToShaderType(m_option.target);
        m_tcontext["ShaderPrefix"] = TargetToShaderPrefix(m_option.target);
        m_tcontext["ShaderPath"] = m_option.shader_path;
        m_tcontext["Entrypoint"] = m_option.entrypoint;
        m_tcontext["Target"] = m_option.target;

        mustache::data tcbuffers{ mustache::data::type::list };

        ComPtr<ID3D11ShaderReflection> reflector;
        D3DReflect(shader_buffer->GetBufferPointer(), shader_buffer->GetBufferSize(), IID_PPV_ARGS(&reflector));

        D3D11_SHADER_DESC desc = {};
        reflector->GetDesc(&desc);

        for (UINT i = 0; i < desc.ConstantBuffers; ++i)
        {
            ID3D11ShaderReflectionConstantBuffer* buffer = reflector->GetConstantBufferByIndex(i);
            D3D11_SHADER_BUFFER_DESC bdesc = {};
            buffer->GetDesc(&bdesc);

            ID3D11ShaderReflectionConstantBuffer* cbuffer = reflector->GetConstantBufferByIndex(i);
            D3D11_SHADER_BUFFER_DESC cbdesc = {};
            cbuffer->GetDesc(&cbdesc);

            mustache::data tcbuffer;
            tcbuffer.set("BufferName", bdesc.Name);
            tcbuffer.set("BufferSize", std::to_string(bdesc.Size));
            tcbuffer.set("BufferIndex", std::to_string(i));
            tcbuffer.set("BufferSeparator", i == 0 ? ":" : ",");

            mustache::data tvariables{ mustache::data::type::list };

            for (UINT i = 0; i < cbdesc.Variables; ++i)
            {
                ID3D11ShaderReflectionVariable* variable = cbuffer->GetVariableByIndex(i);

                D3D11_SHADER_VARIABLE_DESC vdesc;
                variable->GetDesc(&vdesc);

                ID3D11ShaderReflectionType* vtype = variable->GetType();

                D3D11_SHADER_TYPE_DESC type_desc = {};
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
        m_tcontext["CBuffers"] = mustache::data{ tcbuffers };

        mustache::data ttextures{ mustache::data::type::list };
        for (UINT i = 0; i < desc.BoundResources; ++i)
        {
            D3D11_SHADER_INPUT_BIND_DESC res_desc = {};
            ASSERT_SUCCEEDED(reflector->GetResourceBindingDesc(i, &res_desc));
            if (res_desc.Type != D3D_SIT_TEXTURE)
                continue;

            mustache::data ttexture;
            ttexture.set("Name", res_desc.Name);
            ttexture.set("Slot", std::to_string(res_desc.BindPoint));
            ttextures.push_back(ttexture);
        }
        m_tcontext["Textures"] = mustache::data{ ttextures };

        mustache::data tinputs{ mustache::data::type::list };
        for (UINT i = 0; i < desc.InputParameters; ++i)
        {
            D3D11_SIGNATURE_PARAMETER_DESC param_desc = {};
            reflector->GetInputParameterDesc(i, &param_desc);

            mustache::data tinput;
            std::string input_name = param_desc.SemanticName;
            if (param_desc.SemanticIndex)
                input_name += std::to_string(param_desc.SemanticIndex);
            tinput.set("Name", input_name);
            tinput.set("Slot", std::to_string(i));
            tinputs.push_back(tinput);
        }
        m_tcontext["Inputs"] = mustache::data{ tinputs };
    }

    ComPtr<ID3DBlob> CompileShader(const std::string& shader_path, const std::string& entrypoint, const std::string& target)
    {
        ComPtr<ID3DBlob> errors;
        ComPtr<ID3DBlob> shader_buffer;
        ASSERT_SUCCEEDED(D3DCompileFromFile(
            GetAssetFullPathW(shader_path).c_str(),
            nullptr,
            nullptr,
            entrypoint.c_str(),
            target.c_str(),
            0,
            0,
            &shader_buffer,
            &errors));
        return shader_buffer;
    }

    const Option& m_option;
    mustache::mustache m_tmpl;
    mustache::data m_tcontext;
};

class ParseCmd
{
public:
    ParseCmd(int argc, char *argv[])
    {
        ThrowIfFailed(argc == 7, "Invalide CommandLine");
        size_t arg_index = 1;
        m_option.shader_name = argv[arg_index++];
        m_option.shader_path = argv[arg_index++];
        m_option.entrypoint = argv[arg_index++];
        m_option.target = argv[arg_index++];
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
