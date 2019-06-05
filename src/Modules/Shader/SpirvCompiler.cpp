#include "Shader/SpirvCompiler.h"
#include <fstream>
#include <Utilities/FileUtility.h>
#include <cassert>

#ifdef _WIN32
#include "Shader/DXCompiler.h"
#endif

class TmpSpirvFile
{
public:
    TmpSpirvFile()
    {
#ifdef _WIN32
        std::vector<char> tmp_dir(MAX_PATH + 1);
        GetTempPathA(tmp_dir.size(), tmp_dir.data());

        std::vector<char> tmp_file(MAX_PATH + 1);
        GetTempFileNameA(tmp_dir.data(), "SponzaApp.spirv", 0, tmp_file.data());

        m_file_path = tmp_file.data();

        m_handle = CreateFileA(m_file_path.c_str(), // file name
            GENERIC_READ,                           // open for write
            FILE_SHARE_READ | FILE_SHARE_WRITE,     // share
            nullptr,                                // default security
            CREATE_ALWAYS,                          // overwrite existing
            FILE_ATTRIBUTE_NORMAL,                  // normal file
            nullptr);                               // no template

        if (m_handle == INVALID_HANDLE_VALUE)
            throw std::runtime_error("failed to call CreateFileA");
#else
        m_file_path = std::tmpnam(nullptr);
#endif
    }

    const std::string& GetFilePath() const
    {
        return m_file_path;
    }

    ~TmpSpirvFile()
    {
#ifdef _WIN32
        if (m_handle)
            CloseHandle(m_handle);
        DeleteFileA(m_file_path.c_str());
#endif
    }

    std::vector<uint32_t> ReadSpirv()
    {
        std::ifstream file(m_file_path, std::ios::binary);

        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        assert(file_size % 4 == 0);

        std::vector<uint32_t> spirv((file_size + 3) / 4);
        file.read((char*)spirv.data(), file_size);
        return spirv;
    }

private:
    std::string m_file_path;
#ifdef _WIN32
    HANDLE m_handle = nullptr;
#endif
};
#ifdef _WIN32
bool RunProcess(std::string command_line)
{
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessA(nullptr, // No module name (use command line)
        &command_line[0],        // Command line
        nullptr,                 // Process handle not inheritable
        nullptr,                 // Thread handle not inheritable
        false,                   // Set handle inheritance to FALSE
        CREATE_NO_WINDOW,        // Creation flags
        nullptr,                 // Use parent's environment block
        nullptr,                 // Use parent's starting directory 
        &si,                     // Pointer to STARTUPINFO structure
        &pi)                     // Pointer to PROCESS_INFORMATION structure
        )
    {
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return !exit_code;
}
#else
bool RunProcess(std::string command_line)
{
    system(command_line.c_str());
    return true;
}
#endif

std::string GetGlslangPath()
{
#ifdef _WIN32
    const char* vk_sdk_path = getenv("VULKAN_SDK");
    if (!vk_sdk_path)
        return {};
    return std::string(vk_sdk_path) + "/Bin/glslangValidator.exe";
#else
    return "glslangValidator";
#endif
}

std::string MakeCommandLine(const ShaderDesc& shader, const SpirvOption& option, const TmpSpirvFile& spirv_path)
{
    std::string glslang_path = GetGlslangPath();
    if (glslang_path.empty())
        return {};

    std::string shader_type;
    switch (shader.type)
    {
    case ShaderType::kPixel:
        shader_type = "frag";
        break;
    case ShaderType::kVertex:
        shader_type = "vert";
        break;
    case ShaderType::kGeometry:
        shader_type = "geom";
        break;
    case ShaderType::kCompute:
        shader_type = "comp";
        break;
    default:
        return {};
    }

    std::string cmd = glslang_path;
    if (option.auto_map_bindings)
        cmd += " --auto-map-bindings ";
    if (option.hlsl_iomap)
        cmd += " --hlsl-iomap ";
    if (option.resource_set_binding != -1)
        cmd += " --resource-set-binding " + std::to_string(option.resource_set_binding) + " ";
    if (option.invert_y)
        cmd += " --invert-y ";
    cmd += " -g ";
    cmd += " -e ";
    cmd += shader.entrypoint;
    cmd += " -S ";
    cmd += shader_type;
    if (option.vulkan_semantics)
        cmd += " -V ";
    else
        cmd += " -G ";
    cmd += " -D ";
    cmd += " -fhlsl_functionality1 ";
    cmd += GetAssetFullPath(shader.shader_path);
    cmd += " -o ";
    cmd += spirv_path.GetFilePath();

    for (auto &x : shader.define)
    {
        cmd += " -D" + x.first + "=" + x.second;
    }

    return cmd;
}

std::vector<uint32_t> SpirvCompile(const ShaderDesc& shader, const SpirvOption& option)
{
#ifdef _WIN32
    if (option.use_dxc)
    {
        DXOption dx_option = { true, option.invert_y };
        auto blob = DXCompile(shader, dx_option);
        if (!blob)
            return {};
        auto blob_as_uint32 = reinterpret_cast<uint32_t*>(blob->GetBufferPointer());
        std::vector<uint32_t> spirv(blob_as_uint32, blob_as_uint32 + blob->GetBufferSize() / 4);
        return spirv;
    }
#endif

    TmpSpirvFile spirv_path;
    if (!RunProcess(MakeCommandLine(shader, option, spirv_path)))
        return {};
    return spirv_path.ReadSpirv();
}
