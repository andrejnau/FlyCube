#pragma once

#include <Windows.h>
#include <codecvt>
#include <string>

std::wstring utf8_to_wstring(const std::string &str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.from_bytes(str);
}

std::wstring GetAssetFullPath(const std::wstring &assetName)
{
    return utf8_to_wstring(ASSETS_PATH) + assetName;
}