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

std::string GetExecutablePath();
std::string GetExecutableDir();
std::string GetEnv(const std::string& var);
