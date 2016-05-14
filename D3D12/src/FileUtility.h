#pragma once

#include <Windows.h>
#include <string>

#ifndef ASSETS_PATH
#define ASSETS_PATH L"../../src/shaders/"
#endif

std::wstring GetAssetFullPath(LPCWSTR assetName)
{
    return std::wstring(ASSETS_PATH) + assetName;
}