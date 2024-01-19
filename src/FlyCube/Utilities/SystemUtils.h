#pragma once
#include <string>

std::string GetExecutablePath();
std::string GetExecutableDir();
std::string GetEnvironmentVar(const std::string& name);
