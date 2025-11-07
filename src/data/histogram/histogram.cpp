
#include "histogram.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <charconv>
#include <thread>
#include <vector>
#include <array>

namespace fs = std::filesystem;

// --- Helper functions ---
inline bool parseInteger(const char* start, const char* end, int& value) {
    auto result = std::from_chars(start, end, value);
    return result.ec == std::errc{};
}

inline const char* findNextNewline(const char* start, const char* end) {
    while (start < end && *start != '\n') ++start;
    return start;
}

void processChunk(const char* chunkStart, const char* chunkEnd, std::vector<int>& numbers) {
    const char* current = chunkStart;
    numbers.reserve((chunkEnd - chunkStart) / 8); // rough estimate

    while (current < chunkEnd) {
        const char* lineEnd = findNextNewline(current, chunkEnd);
        if (current < lineEnd) {
            int value;
            if (parseInteger(current, lineEnd, value)) {
                numbers.push_back(value);
            }
        }
        current = lineEnd + 1;
    }
}

// --- MainHistogram computation ---
MainHistogram computeHistogramFromFile(const fs::path& filePath) {
    MainHistogram hist;

    if (!fs::exists(filePath)) {
        std::cerr << "File does not exist: " << filePath << "\n";
        return hist;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open file: " << filePath << "\n";
        return hist;
    }

    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string fileContent(fileSize, '\0');
    if (!file.read(fileContent.data(), fileSize)) {
        std::cerr << "Failed to read file: " << filePath << "\n";
        return hist;
    }

    const char* data = fileContent.data();
    const char* dataEnd = data + fileSize;

    const unsigned int numThreads = std::thread::hardware_concurrency();
    const size_t chunkSize = fileSize / numThreads;

    std::vector<std::vector<int>> threadNumbers(numThreads);
    std::vector<std::thread> threads;

    for (unsigned int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&, i]() {
            const char* chunkStart = data + i * chunkSize;
            const char* chunkEnd   = (i == numThreads - 1) ? dataEnd : data + (i + 1) * chunkSize;

            if (i > 0) chunkStart = findNextNewline(chunkStart, dataEnd) + 1;
            if (i < numThreads - 1) chunkEnd = findNextNewline(chunkEnd, dataEnd);

            processChunk(chunkStart, chunkEnd, threadNumbers[i]);
        });
    }
    for (auto& t : threads) t.join();

    std::vector<int> allNumbers;
    size_t totalNumbers = 0;
    for (const auto& nums : threadNumbers) totalNumbers += nums.size();
    allNumbers.reserve(totalNumbers);
    for (auto& nums : threadNumbers) allNumbers.insert(allNumbers.end(), nums.begin(), nums.end());

    if (allNumbers.empty()) {
        std::cerr << "No valid numbers in file: " << filePath << "\n";
        return hist;
    }

    auto percentileIndex = [&](double p) { return static_cast<size_t>(p * (allNumbers.size() - 1)); };

    // 1st percentile
    auto p1Idx = percentileIndex(0.01);
    std::nth_element(allNumbers.begin(), allNumbers.begin() + p1Idx, allNumbers.end());
    int minVal = allNumbers[p1Idx];

    // 99th percentile
    auto p99Idx = percentileIndex(0.99);
    std::nth_element(allNumbers.begin(), allNumbers.begin() + p99Idx, allNumbers.end());
    int maxVal = allNumbers[p99Idx];

    if (minVal >= maxVal) {
        std::cerr << "Invalid percentile min/max\n";
        return hist;
    }

    // Add padding - 5% of range on each side
    int range = maxVal - minVal;
    int padding = static_cast<int>(range * 0.05); // 5% padding
    minVal -= padding;
    maxVal += padding;

    hist.minValue = minVal;
    hist.maxValue = maxVal;
    hist.binWidth = static_cast<double>(maxVal - minVal) / MainHistogram::binCount;

    // --- Fill bins ---
    std::vector<std::array<int, MainHistogram::binCount>> localBins(numThreads);
    const double scaleFactor = static_cast<double>(MainHistogram::binCount) / (maxVal - minVal);

    threads.clear();
    for (unsigned int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&, i]() {
            auto& bins = localBins[i];
            for (int value : threadNumbers[i]) {
                if (value < minVal || value > maxVal) continue;
                int bin = static_cast<int>((value - minVal) * scaleFactor);
                bin = std::clamp(bin, 0, static_cast<int>(MainHistogram::binCount - 1));
                bins[bin]++;
            }
        });
    }
    for (auto& t : threads) t.join();

    for (const auto& localBin : localBins) {
        for (size_t i = 0; i < MainHistogram::binCount; ++i) hist.binCounts[i] += localBin[i];
    }

    // Gaussian smoothing for smoother histogram rendering
    {
        std::vector<double> smoothed(MainHistogram::binCount);
        const double sigma = 1.5;  // tweak for more/less smoothing (1.0 = subtle, 2.5 = very smooth)
        const int radius = static_cast<int>(std::ceil(3 * sigma));

        for (size_t i = 0; i < MainHistogram::binCount; ++i) {
            double sum = 0.0;
            double weightSum = 0.0;
            for (int j = -radius; j <= radius; ++j) {
                int idx = static_cast<int>(i) + j;
                if (idx >= 0 && idx < MainHistogram::binCount) {
                    double w = std::exp(-0.5 * (j * j) / (sigma * sigma));
                    sum += static_cast<double>(hist.binCounts[idx]) * w;
                    weightSum += w;
                }
            }
            smoothed[i] = sum / weightSum;
        }

        // Copy back to int bins (or store separately if you want a toggle)
        for (size_t i = 0; i < MainHistogram::binCount; ++i)
            hist.binCounts[i] = static_cast<int>(smoothed[i]);
    }

    return hist;
}
