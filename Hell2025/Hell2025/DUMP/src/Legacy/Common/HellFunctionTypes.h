#pragma once
#include <string>

using NewFileCallback = void(*)(const std::string&);
using OpenFileCallback = void(*)(const std::string&);
