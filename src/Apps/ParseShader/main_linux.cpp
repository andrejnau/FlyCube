#include <Utilities/FileUtility.h>
#include <Shader/SpirvCompiler.h>
#include <Shader/ShaderBase.h>
#include <spirv_cross.hpp>
#include <spirv_hlsl.hpp>
#include <mustache.hpp>
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <fstream>
#include <iterator>
#include <cctype>
#include <map>

class ShaderReflection
{
public:
    ShaderReflection(const ShaderDesc& shader_desc, const std::string& shader_name)
        : m_shader_desc(shader_desc)
        , m_shader_name(shader_name)
    {
        Parse();
    }

    void Gen(const std::string& template_path, const std::string& output_dir)
    {
        kainjow::mustache::mustache tmpl(ReadFile(template_path));
        std::stringstream buf;
        tmpl.render(m_tcontext, buf);
        std::string new_content = buf.str();

        std::ifstream is(output_dir + "/" + m_shader_name + ".h");
        std::string old_content((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());

        if (new_content != old_content)
        {
#ifdef _WIN32
            throw std::runtime_error("different generated code");
#else
            std::ofstream os(output_dir + "/" + m_shader_name + ".h");
            os << new_content;
#endif
        }
    }

private:
    void Parse()
    {
        spirv_cross::CompilerHLSL compiler(SpirvCompile(m_shader_desc, {}));
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        m_tcontext["ShaderName"] = m_shader_name;
        m_tcontext["ShaderType"] = ShaderTypeToString(m_shader_desc.type);
        m_tcontext["ShaderPrefix"] = TargetToShaderPrefix(m_shader_desc.target);
        m_tcontext["ShaderPath"] = m_shader_desc.shader_path;
        m_tcontext["Entrypoint"] = m_shader_desc.entrypoint;
        m_tcontext["Target"] = m_shader_desc.target;

        kainjow::mustache::data tcbuffers{ kainjow::mustache::data::type::list };

        for (const auto& cbuffer : resources.uniform_buffers)
        {
            auto& type = compiler.get_type(cbuffer.type_id);
            if (type.basetype != spirv_cross::SPIRType::BaseType::Struct)
                continue;

            kainjow::mustache::data tcbuffer;
            tcbuffer.set("BufferName", cbuffer.name);
            tcbuffer.set("BufferSize", std::to_string(compiler.get_declared_struct_size(type)));
            tcbuffer.set("BufferIndex", std::to_string(compiler.get_decoration(cbuffer.id, spv::DecorationBinding)));
            tcbuffer.set("BufferSeparator", tcbuffers.is_empty_list() ? ":" : ",");

            kainjow::mustache::data tvariables{ kainjow::mustache::data::type::list };

            for (uint32_t i = 0; i < type.member_types.size(); ++i)
            {
                auto& field_type = compiler.get_type(type.member_types[i]);

                kainjow::mustache::data tvariable;
                tvariable.set("Name", compiler.get_member_name(cbuffer.base_type_id, i));
                tvariable.set("StartOffset", std::to_string(compiler.type_struct_member_offset(type, i)));
                tvariable.set("VariableSize", std::to_string(compiler.get_declared_struct_member_size(type, i)));

                std::string type_prefix;
                std::string shader_type_to_cpp_type;
                switch (field_type.basetype)
                {
                case spirv_cross::SPIRType::BaseType::Int:
                    type_prefix = "i";
                    break;
                case spirv_cross::SPIRType::BaseType::UInt:
                    type_prefix = "u";
                    break;
                case spirv_cross::SPIRType::BaseType::Boolean:
                    type_prefix = "b";
                    break;
                }
                if (field_type.columns == 1)
                    shader_type_to_cpp_type = "glm::" + type_prefix + "vec" + std::to_string(field_type.vecsize);
                else
                    shader_type_to_cpp_type = "glm::" + type_prefix + "mat" + std::to_string(field_type.vecsize) + "x" + std::to_string(field_type.columns);
                tvariable.set("Type", shader_type_to_cpp_type);

                tvariables.push_back(tvariable);
            }
            tcbuffer.set("Variables", tvariables);

            tcbuffers.push_back(tcbuffer);
        }
        m_tcontext["CBuffers"] = kainjow::mustache::data{ tcbuffers };

        kainjow::mustache::data ttextures{ kainjow::mustache::data::type::list };
        kainjow::mustache::data tuavs{ kainjow::mustache::data::type::list };
        kainjow::mustache::data tsamplers{ kainjow::mustache::data::type::list };

        auto add_resources = [](const spirv_cross::Compiler& compiler, const spirv_cross::SmallVector<spirv_cross::Resource>& resources, kainjow::mustache::data& tdata)
        {
            for (const auto& resource : resources)
            {
                kainjow::mustache::data tresource;
                tresource.set("Name", resource.name);
                tresource.set("Slot", std::to_string(compiler.get_decoration(resource.id, spv::DecorationBinding)));
                tresource.set("Separator", tdata.is_empty_list() ? ":" : ",");
                tdata.push_back(tresource);
                break;
            }
        };

        add_resources(compiler, resources.separate_images, ttextures);
        add_resources(compiler, resources.storage_images, tuavs);
        add_resources(compiler, resources.storage_buffers, tuavs);
        add_resources(compiler, resources.separate_samplers, tsamplers);

        m_tcontext["Textures"] = kainjow::mustache::data{ ttextures };
        m_tcontext["UAVs"] = kainjow::mustache::data{ tuavs };
        m_tcontext["Samplers"] = kainjow::mustache::data{ tsamplers };

        if (m_shader_desc.type == ShaderType::kVertex)
        {
            kainjow::mustache::data tinputs{ kainjow::mustache::data::type::list };
            for (const auto& resource : resources.stage_inputs)
            {
                kainjow::mustache::data tinput;
                tinput.set("Name", compiler.get_decoration_string(resource.id, spv::DecorationHlslSemanticGOOGLE));
                tinput.set("Slot", std::to_string(compiler.get_decoration(resource.id, spv::DecorationLocation)));
                tinputs.push_back(tinput);
            }
            m_tcontext["Inputs"] = kainjow::mustache::data{ tinputs };
        }

        if (m_shader_desc.type == ShaderType::kPixel)
        {
            kainjow::mustache::data toutputs{ kainjow::mustache::data::type::list };

            for (const auto& resource : resources.stage_outputs)
            {
                kainjow::mustache::data toutput;
                toutput.set("Slot", std::to_string(compiler.get_decoration(resource.id, spv::DecorationLocation)));
                toutput.set("Separator", toutputs.is_empty_list() ? ":" : ",");
                toutputs.push_back(toutput);
            }
            m_tcontext["Outputs"] = kainjow::mustache::data{ toutputs };
            m_tcontext["DSVSeparator"] = toutputs.is_empty_list() ? ":" : ",";
            m_tcontext["HasOutputs"] = true;
        }
    }

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
        case ShaderType::kLibrary:
            return "ShaderType::kLibrary";
        default:
            return "ShaderType::kUnknown";
        }
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

    std::string ReadFile(const std::string& path)
    {
        std::ifstream is(path);
        return std::string(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
    }

    ShaderDesc m_shader_desc;
    const std::string& m_shader_name;
    kainjow::mustache::data m_tcontext;
};

class ParseCmd
{
public:
    ParseCmd(int argc, char *argv[])
    {
        if (argc != 8)
            throw std::runtime_error("Invalide CommandLine");
        size_t arg_index = 1;
        m_shader_name = argv[arg_index++];
        m_shader_path = argv[arg_index++];
        m_entrypoint = argv[arg_index++];
        m_type = argv[arg_index++];
        m_model = argv[arg_index++];
        m_template_path = argv[arg_index++];
        m_output_dir = argv[arg_index++];
    }

    const std::string& GetShaderName() const
    {
        return m_shader_name;
    }

    const std::string& GetTemplatePath() const
    {
        return m_template_path;
    }

    const std::string& GetOutputDir() const
    {
        return m_output_dir;
    }

    ShaderDesc GetShaderDesc() const
    {
        std::string target;
        std::string entrypoint;
        if (m_type == "Library")
        {
            target = "lib_" + m_model;
            target.replace(target.find("."), 1, "_");
        }
        else
        {
            target = "xs_" + m_model;
            target.replace(target.find("."), 1, "_");
            target.front() = std::tolower(m_type[0]);
            entrypoint = m_entrypoint;
        }
        return ShaderDesc(m_shader_path, entrypoint, target);
    }

private:
    std::string m_shader_name;
    std::string m_shader_path;
    std::string m_entrypoint;
    std::string m_type;
    std::string m_model;
    std::string m_template_path;
    std::string m_output_dir;
};

int main(int argc, char *argv[]) try
{
    ParseCmd cmd(argc, argv);
    ShaderReflection ref(cmd.GetShaderDesc(), cmd.GetShaderName());
    ref.Gen(cmd.GetTemplatePath(), cmd.GetOutputDir());
    return 0;
}
catch (std::exception&)
{
    return ~0;
}
