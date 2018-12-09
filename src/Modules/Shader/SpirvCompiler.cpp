#include "Shader/SpirvCompiler.h"
#include <fstream>
#include <Windows.h>
#include <Utilities/FileUtility.h> 
#include <cassert>

class TmpSpirvFile
{
public:
    TmpSpirvFile()
    {
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
    }

    const std::string& GetFilePath() const
    {
        return m_file_path;
    }

    ~TmpSpirvFile()
    {
        if (m_handle)
            CloseHandle(m_handle);
        DeleteFileA(m_file_path.c_str());
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
    HANDLE m_handle = nullptr;
};

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

std::string MakeCommandLine(const ShaderBase& shader, const SpirvOption& option, const TmpSpirvFile& spirv_path)
{
    const char* vk_sdk_path = getenv("VULKAN_SDK");
    if (!vk_sdk_path)
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

    std::string cmd = std::string(vk_sdk_path) + "/Bin/glslangValidator.exe";
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
    cmd += " -V -D ";
    cmd += GetAssetFullPath(shader.shader_path);
    cmd += " -o ";
    cmd += spirv_path.GetFilePath();

    for (auto &x : shader.define)
    {
        cmd += " -D" + x.first + "=" + x.second;
    }

    return cmd;
}

std::vector<uint32_t> SpirvCompile(const ShaderBase& shader, const SpirvOption& option)
{
    TmpSpirvFile spirv_path;
    if (!RunProcess(MakeCommandLine(shader, option, spirv_path)))
        return {};

    return spirv_path.ReadSpirv();
}
