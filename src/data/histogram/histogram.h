
#pragma once

#include <array>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

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
    static constexpr int binCount = 2500;
    double minValue = 0.0;
    double maxValue = 0.0;
    double binWidth = 0.0;
    std::array<int, binCount> binCounts{};
};

// Helper to read the first number from a line
bool parseFirstNumber(const std::string& line, double& value);

// Compute histogram from a file
PrecomputedHistogram computeHistogramFromFile(const fs::path& filePath);