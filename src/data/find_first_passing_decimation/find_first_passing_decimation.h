
#pragma once

#include <filesystem>

#include "../data_manager.h"
#include "../../core/types.h"


namespace fs = std::filesystem;
bool findFirstPassingDecimation(const fs::path& filepath);

//bool findFirstPassingDecimation(const std::filesystem::path& filepath);