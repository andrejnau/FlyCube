#pragma once

#include <codecvt>
#include <locale>
#include <string>
#include <fstream>
#include <Windows.h>

inline std::wstring utf8_to_wstring(const std::string &str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.from_bytes(str);
}

inline std::string wstring_to_utf8(const std::wstring &str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.to_bytes(str);
}

inline std::wstring GetAssetFullPath(const std::wstring& assetName)
{
    std::wstring path = utf8_to_wstring(ASSETS_PATH) + assetName;
    if (!std::ifstream(wstring_to_utf8(path)).good())
        path = L"assets/" + assetName;
    return path;
}

inline std::string GetAssetFullPath(const std::string& assetName)
{
    return wstring_to_utf8(GetAssetFullPath(utf8_to_wstring(assetName)));
}

inline std::wstring GetAssetFullPathW(const std::string& assetName)
{
    return GetAssetFullPath(utf8_to_wstring(assetName));
}

inline std::string GetExecutablePath()
{
    char buf[MAX_PATH + 1];
    GetModuleFileNameA(nullptr, buf, sizeof(buf));
    return buf;
}

inline std::string GetExecutableDir()
{
    auto path = GetExecutablePath();
    return path.substr(0, path.find_last_of("\\/"));
}
