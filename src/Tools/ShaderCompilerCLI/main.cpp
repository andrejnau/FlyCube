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
    if (blob.empty()) {
        exit(~0);
    }
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
        shader_name_ = get_next_arg();
        desc_.shader_path = get_next_arg();
        desc_.entrypoint = get_next_arg();
        desc_.type = GetShaderType(get_next_arg());
        desc_.model = get_next_arg();
        output_dir_ = get_next_arg();
    }

    const std::string& GetShaderName() const
    {
        return shader_name_;
    }

    const ShaderDesc& GetShaderDesc() const
    {
        return desc_;
    }

    const std::string& GetOutputDir() const
    {
        return output_dir_;
    }

private:
    std::string shader_name_;
    ShaderDesc desc_;
    std::string output_dir_;
};

int main(int argc, char* argv[])
{
    ParseCmd cmd(argc, argv);
    CompileShader(cmd.GetShaderName(), cmd.GetShaderDesc(), cmd.GetOutputDir(), ShaderBlobType::kDXIL);
    CompileShader(cmd.GetShaderName(), cmd.GetShaderDesc(), cmd.GetOutputDir(), ShaderBlobType::kSPIRV);
    return 0;
}
