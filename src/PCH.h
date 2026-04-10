// SSSOAutoConfig — Precompiled header
// Author: Young Jonii

#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <spdlog/sinks/basic_file_sink.h>

#include <ShlObj.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using namespace std::string_literals;
using namespace std::string_view_literals;
