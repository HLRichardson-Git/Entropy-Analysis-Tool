
#pragma once

#include <array>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <filesystem>
#include <iostream>

#include "../../core/thread_pool/thread_pool.h"
#include "../../core/types.h"

namespace fs = std::filesystem;

// Compute histogram from a file
MainHistogram computeHistogramFromFile(const fs::path& filePath);