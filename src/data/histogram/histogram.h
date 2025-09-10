
#pragma once

#include "../../core/thread_pool/thread_pool.h"

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

namespace fs = std::filesystem;

struct PrecomputedHistogram {
    static constexpr int binCount = 1500;
    unsigned int minValue = 0;
    unsigned int maxValue = 0;
    double binWidth = 0.0;
    std::array<int, binCount> binCounts{};
};

// Compute histogram from a file
PrecomputedHistogram computeHistogramFromFile(const fs::path& filePath);