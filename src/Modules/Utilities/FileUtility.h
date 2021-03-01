#pragma once
#include <codecvt>
#include <locale>
#include <string>
#include <fstream>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

std::wstring utf8_to_wstring(const std::string& str);
std::string wstring_to_utf8(const std::wstring& str);
std::string GetExecutablePath();
std::string GetExecutableDir();
std::wstring GetAssetFullPath(const std::wstring& assetName);
std::string GetAssetFullPath(const std::string& assetName);
std::wstring GetAssetFullPathW(const std::string& assetName);
std::string GetEnv(const std::string& var);
