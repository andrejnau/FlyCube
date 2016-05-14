#pragma once

#include <Windows.h>
#include <string>

std::wstring GetAssetFullPath(LPCWSTR assetName)
{
    return std::wstring(ASSETS_PATH) + assetName;
}