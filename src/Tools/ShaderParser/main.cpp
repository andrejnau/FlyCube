#include "ShaderParser.h"
#include <iostream>
#include <fstream>
#include <cassert>

inline void ThrowIfFailed(bool res, const std::string& msg)
{
    if (!res)
        throw std::runtime_error(msg);
}

std::string ReadFile(const std::string& path)
{
    std::ifstream is(path);
    return std::string(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
}

class ParseCmd
{
public:
    ParseCmd(int argc, char *argv[])
    {
        ThrowIfFailed(argc == 10, "Invalide CommandLine");
        size_t arg_index = 1;
        m_option.assets_path = argv[arg_index++];
        m_option.shader_name = argv[arg_index++];
        m_option.shader_path = argv[arg_index++];
        m_option.entrypoint = argv[arg_index++];
        std::string type = argv[arg_index++];
        std::string model = argv[arg_index++];
        std::string template_path = argv[arg_index++];
        m_output_dir = argv[arg_index++];
        m_build_folder = argv[arg_index++];

        model.replace(model.find("."), 1, "_");
        m_option.blob_type = ShaderBlobType::kDXIL;
        m_option.type = GetShaderType(type);
        m_option.model = model;
        m_option.mustache_template = ReadFile(template_path);
    }

    ShaderType GetShaderType(const std::string& target)
    {
        if (target == "Pixel")
            return ShaderType::kPixel;
        else if (target == "Vertex")
            return ShaderType::kVertex;
        else if (target == "Compute")
            return ShaderType::kCompute;
        else if (target == "Geometry")
            return ShaderType::kGeometry;
        else if (target == "Amplification")
            return ShaderType::kAmplification;
        else if (target == "Mesh")
            return ShaderType::kMesh;
        else if (target == "Library")
            return ShaderType::kLibrary;
        assert(false);
        return ShaderType::kUnknown;
    }

    const Option& GetOption() const
    {
        return m_option;
    }

    const std::string& GetOutputDir() const
    {
        return m_output_dir;
    }

    const std::string& GetBuildFolder() const
    {
        return m_build_folder;
    }

private:
    Option m_option;
    std::string m_output_dir;
    std::string m_build_folder;
};

int main(int argc, char *argv[])
{
    try
    {
        ParseCmd cmd(argc, argv);

        std::string path = cmd.GetOutputDir() + "/" + cmd.GetOption().shader_name + ".h";
        std::string old_content = ReadFile(path);
        std::string new_content = RenderShaderReflection(cmd.GetOption());
        if (new_content != old_content)
        {
            std::ofstream os(path);
            os << new_content;
        }
    }
    catch (std::exception&)
    {
        std::cout << "TODO" << std::endl;
        return ~0;
    }
    return 0;
}
