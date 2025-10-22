#include "HLSLCompiler/Compiler.h"
#include "Instance/BaseTypes.h"
#include "Utilities/NotReached.h"

#include <cassert>
#include <fstream>

namespace {

ShaderType GetShaderType(const std::string& target)
{
    if (target == "Pixel") {
        return ShaderType::kPixel;
    } else if (target == "Vertex") {
        return ShaderType::kVertex;
    } else if (target == "Compute") {
        return ShaderType::kCompute;
    } else if (target == "Geometry") {
        return ShaderType::kGeometry;
    } else if (target == "Amplification") {
        return ShaderType::kAmplification;
    } else if (target == "Mesh") {
        return ShaderType::kMesh;
    } else if (target == "Library") {
        return ShaderType::kLibrary;
    }
    NOTREACHED();
}

std::string GetShaderExtension(ShaderBlobType shader_type)
{
    switch (shader_type) {
    case ShaderBlobType::kDXIL:
        return ".dxil";
    case ShaderBlobType::kSPIRV:
        return ".spirv";
    default:
        NOTREACHED();
    }
}

void CompileShader(const std::string& shader_name,
                   const ShaderDesc& desc,
                   const std::string& output_path,
                   ShaderBlobType shader_type)
{
    std::string path = output_path + "/" + shader_name + GetShaderExtension(shader_type);
    std::vector<uint8_t> blob = Compile(desc, shader_type);
    std::fstream file(path, std::ios::out | std::ios::binary);
    file.write(reinterpret_cast<char*>(blob.data()), blob.size());
}

} // namespace

class ParseCmd {
public:
    ParseCmd(int argc, char* argv[])
    {
        size_t arg_index = 0;
        auto get_next_arg = [&] {
            ++arg_index;
            assert(arg_index < argc);
            return argv[arg_index];
        };
        m_shader_name = get_next_arg();
        m_desc.shader_path = get_next_arg();
        m_desc.entrypoint = get_next_arg();
        m_desc.type = GetShaderType(get_next_arg());
        m_desc.model = get_next_arg();
        m_output_dir = get_next_arg();
    }

    const std::string& GetShaderName() const
    {
        return m_shader_name;
    }

    const ShaderDesc& GetShaderDesc() const
    {
        return m_desc;
    }

    const std::string& GetOutputDir() const
    {
        return m_output_dir;
    }

private:
    std::string m_shader_name;
    ShaderDesc m_desc;
    std::string m_output_dir;
};

int main(int argc, char* argv[])
{
    ParseCmd cmd(argc, argv);
    CompileShader(cmd.GetShaderName(), cmd.GetShaderDesc(), cmd.GetOutputDir(), ShaderBlobType::kDXIL);
    CompileShader(cmd.GetShaderName(), cmd.GetShaderDesc(), cmd.GetOutputDir(), ShaderBlobType::kSPIRV);
    return 0;
}
