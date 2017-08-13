#pragma once

#include <Windows.h>
#include <fstream>
#include <codecvt>
#include <string>

inline std::wstring utf8_to_wstring(const std::string &str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.from_bytes(str);
}

inline std::wstring GetAssetFullPath(const std::wstring &assetName)
{
    std::wstring path = utf8_to_wstring(ASSETS_PATH) + assetName;
    if (!std::wifstream(path).good())
        path = L"assets/" + assetName;
    return path;
}