
#include "histogram.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <charconv>
#include <thread>
#include <vector>
#include <array>

inline bool parseInteger(const char* start, const char* end, int& value) {
    auto result = std::from_chars(start, end, value);
    return result.ec == std::errc{};
}

// Find next newline character
inline const char* findNextNewline(const char* start, const char* end) {
    while (start < end && *start != '\n') {
        ++start;
    }
    return start;
}

// Process a chunk of data and extract integers
void processChunk(const char* chunkStart, const char* chunkEnd, 
                 std::vector<int>& numbers) {
    const char* current = chunkStart;
    
    // Reserve space to reduce reallocations
    numbers.reserve((chunkEnd - chunkStart) / 8); // Rough estimate
    
    while (current < chunkEnd) {
        const char* lineEnd = findNextNewline(current, chunkEnd);
        
        if (current < lineEnd) {
            int value;
            if (parseInteger(current, lineEnd, value)) {
                numbers.push_back(value);
            }
        }
        
        current = lineEnd + 1; // Skip the newline
    }
}

PrecomputedHistogram computeHistogramFromFile(const fs::path& filePath) {
    PrecomputedHistogram hist;
    
    if (!fs::exists(filePath)) {
        std::cerr << "File does not exist: " << filePath << "\n";
        return hist;
    }
    
    // Read entire file into memory
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
    file.close();
    
    const char* data = fileContent.data();
    const char* dataEnd = data + fileSize;
    
    // --- Pass 1: Parse all numbers in parallel to find min/max ---
    const unsigned int numThreads = std::thread::hardware_concurrency();
    const size_t chunkSize = fileSize / numThreads;
    
    std::vector<std::vector<int>> threadNumbers(numThreads);
    std::vector<std::thread> threads;
    
    for (unsigned int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&, i]() {
            const char* chunkStart = data + (i * chunkSize);
            const char* chunkEnd = (i == numThreads - 1) ? dataEnd : data + ((i + 1) * chunkSize);
            
            // Adjust chunk boundaries to line boundaries (except for first/last)
            if (i > 0) {
                // Move start to beginning of next line
                chunkStart = findNextNewline(chunkStart, dataEnd);
                if (chunkStart < dataEnd) chunkStart++; // Skip the newline
            }
            if (i < numThreads - 1) {
                // Move end to end of current line
                chunkEnd = findNextNewline(chunkEnd, dataEnd);
            }
            
            processChunk(chunkStart, chunkEnd, threadNumbers[i]);
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Find global min/max (positive integers only)
    int minVal = std::numeric_limits<int>::max();
    int maxVal = 0;  // Start at 0 since all values are positive
    size_t totalNumbers = 0;
    
    for (const auto& numbers : threadNumbers) {
        totalNumbers += numbers.size();
        if (!numbers.empty()) {
            auto [localMin, localMax] = std::minmax_element(numbers.begin(), numbers.end());
            minVal = std::min(minVal, *localMin);
            maxVal = std::max(maxVal, *localMax);
        }
    }
    
    if (minVal >= maxVal || totalNumbers == 0) {
        std::cerr << "Invalid min/max or no valid numbers in file: " << filePath << "\n";
        return hist;
    }
    
    // Convert to double for histogram compatibility
    hist.minValue = static_cast<double>(minVal);
    hist.maxValue = static_cast<double>(maxVal);
    hist.binWidth = static_cast<double>(maxVal - minVal) / PrecomputedHistogram::binCount;
    
    // --- Pass 2: Fill bins in parallel using integer arithmetic ---
    std::vector<std::array<int, PrecomputedHistogram::binCount>> localBins(numThreads);
    
    // For integer optimization: precompute scaling factor
    const int range = maxVal - minVal;
    const double scaleFactor = static_cast<double>(PrecomputedHistogram::binCount) / range;
    
    threads.clear();
    for (unsigned int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&, i]() {
            const auto& numbers = threadNumbers[i];
            auto& bins = localBins[i];
            
            for (int value : numbers) {
                // Fast integer-based bin calculation
                int bin;
                if (range > 0) {
                    bin = static_cast<int>((value - minVal) * scaleFactor);
                    // Clamp to valid range
                    bin = std::clamp(bin, 0, static_cast<int>(PrecomputedHistogram::binCount - 1));
                } else {
                    bin = 0; // All values are the same
                }
                bins[bin]++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Combine local bins
    for (const auto& localBin : localBins) {
        for (size_t i = 0; i < PrecomputedHistogram::binCount; ++i) {
            hist.binCounts[i] += localBin[i];
        }
    }
    
    return hist;
}