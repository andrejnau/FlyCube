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
#include <set>

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
        tmpl.render(m_tdevice, buf);
        std::string new_content = buf.str();

        std::ifstream is(output_dir + "/" + m_shader_name + ".h");
        std::string old_content((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());

        if (new_content != old_content)
        {
            std::ofstream os(output_dir + "/" + m_shader_name + ".h");
            os << new_content;
        }
    }

private:
    ViewType GetViewType(const spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& type, uint32_t resource_id)
    {
        switch (type.basetype)
        {
        case spirv_cross::SPIRType::SampledImage:
        {
            return ViewType::kShaderResource;
        }
        case spirv_cross::SPIRType::Image:
        {
            if (type.image.sampled == 2 && type.image.dim != spv::DimSubpassData)
                return ViewType::kUnorderedAccess;
            else
                return ViewType::kShaderResource;
        }
        case spirv_cross::SPIRType::Sampler:
        {
            return ViewType::kSampler;
        }
        case spirv_cross::SPIRType::Struct:
        {
            if (type.storage == spv::StorageClassUniform || type.storage == spv::StorageClassStorageBuffer)
            {
                if (compiler.has_decoration(type.self, spv::DecorationBufferBlock))
                {
                    spirv_cross::Bitset flags = compiler.get_buffer_block_flags(resource_id);
                    bool is_readonly = flags.get(spv::DecorationNonWritable);
                    if (is_readonly)
                        return ViewType::kShaderResource;
                    else
                        return ViewType::kUnorderedAccess;
                }
                else if (compiler.has_decoration(type.self, spv::DecorationBlock))
                {
                    return ViewType::kConstantBuffer;
                }
            }
            else if (type.storage == spv::StorageClassPushConstant)
            {
                return ViewType::kConstantBuffer;
            }
            else
            {
                return ViewType::kUnknown;
            }
        }
        default:
            return ViewType::kUnknown;
        }
    }

    std::string GenerateCppType(const spirv_cross::SPIRType& type)
    {
        std::string glm_type_prefix;
        std::string base_type;
        switch (type.basetype)
        {
        case spirv_cross::SPIRType::BaseType::Float:
            base_type = "float";
            break;
        case spirv_cross::SPIRType::BaseType::Int:
            base_type = "int32_t";
            glm_type_prefix = "i";
            break;
        case spirv_cross::SPIRType::BaseType::UInt:
            base_type = "uint32_t";
            glm_type_prefix = "u";
            break;
        case spirv_cross::SPIRType::BaseType::Boolean:
            base_type = "uint32_t";
            glm_type_prefix = "b";
            break;
        }

        std::string cpp_type;
        if (type.vecsize > 1 && type.columns > 1)
        {
            // TODO: check row_major/column_major
            cpp_type = "glm::" + glm_type_prefix + "mat" + std::to_string(type.vecsize) + "x" + std::to_string(type.columns);
        }
        else if (type.vecsize > 1)
        {
            cpp_type = "glm::" + glm_type_prefix + "vec" + std::to_string(type.vecsize);
        }
        else
        {
            cpp_type = base_type;
        }

        bool type_is_array = !type.array.empty();
        if (type_is_array)
            cpp_type = "std::array<" + cpp_type + ", " + std::to_string(type.array.front()) + ">";

        return cpp_type;
    }

    void Parse()
    {
        SpirvOption option = {};
        option.auto_map_bindings = true;
        option.hlsl_iomap = true;
        option.fhlsl_functionality1 = true;
        spirv_cross::CompilerHLSL compiler(SpirvCompile(m_shader_desc, option));
        spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        m_tdevice["ShaderName"] = m_shader_name;
        m_tdevice["ShaderType"] = ShaderTypeToString(m_shader_desc.type);
        m_tdevice["ShaderPrefix"] = TypeToShaderPrefix(m_shader_desc.type);
        m_tdevice["ShaderPath"] = m_shader_desc.shader_path;
        m_tdevice["Entrypoint"] = m_shader_desc.entrypoint;
        m_tdevice["Target"] = m_shader_desc.target;

        kainjow::mustache::data tcbuffers{ kainjow::mustache::data::type::list };

        for (const auto& cbuffer : resources.uniform_buffers)
        {
            auto& type = compiler.get_type(cbuffer.type_id);
            ViewType view_type = GetViewType(compiler, compiler.get_type(cbuffer.type_id), cbuffer.id);
            if (view_type != ViewType::kConstantBuffer)
                throw std::runtime_error("wrong resource type");

            kainjow::mustache::data tcbuffer;
            tcbuffer.set("BufferName", cbuffer.name);
            tcbuffer.set("BufferSize", std::to_string(compiler.get_declared_struct_size(type)));
            tcbuffer.set("BufferIndex", std::to_string(compiler.get_decoration(cbuffer.id, spv::DecorationBinding)));
            tcbuffer.set("BufferSeparator", tcbuffers.is_empty_list() ? ":" : ",");

            kainjow::mustache::data tvariables{ kainjow::mustache::data::type::list };

            for (uint32_t i = 0; i < type.member_types.size(); ++i)
            {
                kainjow::mustache::data tvariable;
                tvariable.set("Name", compiler.get_member_name(cbuffer.base_type_id, i));
                tvariable.set("StartOffset", std::to_string(compiler.type_struct_member_offset(type, i)));
                tvariable.set("VariableSize", std::to_string(compiler.get_declared_struct_member_size(type, i)));
                tvariable.set("Type", GenerateCppType(compiler.get_type(type.member_types[i])));
                tvariables.push_back(tvariable);
            }
            tcbuffer.set("Variables", tvariables);
            tcbuffers.push_back(tcbuffer);
        }
        m_tdevice["CBuffers"] = kainjow::mustache::data{ tcbuffers };

        kainjow::mustache::data ttextures{ kainjow::mustache::data::type::list };
        kainjow::mustache::data tuavs{ kainjow::mustache::data::type::list };
        kainjow::mustache::data tsamplers{ kainjow::mustache::data::type::list };

        std::set<std::string> used_names;

        auto add_resources = [&](const spirv_cross::Compiler& compiler, const spirv_cross::SmallVector<spirv_cross::Resource>& resources)
        {
            for (const auto& resource : resources)
            {
                std::string name = compiler.get_name(resource.id);
                if (used_names.count(name))
                    throw std::runtime_error("wrong name");
                used_names.insert(name);

                ViewType view_type = GetViewType(compiler, compiler.get_type(resource.type_id), resource.id);

                kainjow::mustache::data* tdata = nullptr;
                switch (view_type)
                {
                case ViewType::kShaderResource:
                    tdata = &ttextures;
                    break;
                case ViewType::kUnorderedAccess:
                    tdata = &tuavs;
                    break;
                case ViewType::kSampler:
                    tdata = &tsamplers;
                    break;
                default:
                    throw std::runtime_error("wrong resource type");
                }

                kainjow::mustache::data tresource;
                tresource.set("Name", name);
                tresource.set("Slot", std::to_string(compiler.get_decoration(resource.id, spv::DecorationBinding)));
                tresource.set("Separator", tdata->is_empty_list() ? ":" : ",");
                tdata->push_back(tresource);
            }
        };

        add_resources(compiler, resources.separate_images);
        add_resources(compiler, resources.storage_images);
        add_resources(compiler, resources.storage_buffers);
        add_resources(compiler, resources.separate_samplers);

        m_tdevice["Textures"] = kainjow::mustache::data{ ttextures };
        m_tdevice["UAVs"] = kainjow::mustache::data{ tuavs };
        m_tdevice["Samplers"] = kainjow::mustache::data{ tsamplers };

        if (m_shader_desc.type == ShaderType::kVertex)
        {
            kainjow::mustache::data tinputs{ kainjow::mustache::data::type::list };
            for (const auto& resource : resources.stage_inputs)
            {
                kainjow::mustache::data tinput;
                std::string semantic = compiler.get_decoration_string(resource.id, spv::DecorationHlslSemanticGOOGLE);
                std::string semantic_slot;
                while (!semantic.empty() && std::isdigit(semantic.back()))
                {
                    semantic_slot = semantic.back() + semantic_slot;
                    semantic.pop_back();
                }
                if (semantic_slot != "0")
                    semantic += semantic_slot;
                tinput.set("Name", semantic);
                tinput.set("Slot", std::to_string(compiler.get_decoration(resource.id, spv::DecorationLocation)));
                tinputs.push_back(tinput);
            }
            m_tdevice["Inputs"] = kainjow::mustache::data{ tinputs };
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
            m_tdevice["Outputs"] = kainjow::mustache::data{ toutputs };
            m_tdevice["DSVSeparator"] = toutputs.is_empty_list() ? ":" : ",";
            m_tdevice["HasOutputs"] = true;
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

    std::string TypeToShaderPrefix(ShaderType type)
    {
        switch (type)
        {
        case ShaderType::kVertex:
            return "VS";
        case ShaderType::kPixel:
            return "PS";
        case ShaderType::kCompute:
            return "CS";
        case ShaderType::kGeometry:
            return "GS";
        default:
            return "";
        }
    }

    std::string ReadFile(const std::string& path)
    {
        std::ifstream is(path);
        return std::string(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
    }

    ShaderDesc m_shader_desc;
    const std::string& m_shader_name;
    kainjow::mustache::data m_tdevice;
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
