
#include "histogram.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>

// Helper to read the first number from a line
inline bool parseFirstNumber(const std::string& line, double& value) {
    std::istringstream iss(line);
    return bool(iss >> value);
}

// Compute histogram from a file
PrecomputedHistogram computeHistogramFromFile(const fs::path& filePath) {
    PrecomputedHistogram hist;

    if (!fs::exists(filePath)) {
        std::cerr << "File does not exist: " << filePath << "\n";
        return hist;
    }

    // --- Pass 1: Determine min/max ---
    double minVal = std::numeric_limits<double>::max();
    double maxVal = std::numeric_limits<double>::lowest();

    {
        std::ifstream in(filePath);
        std::string line;
        while (std::getline(in, line)) {
            double value;
            if (parseFirstNumber(line, value)) {
                minVal = std::min(minVal, value);
                maxVal = std::max(maxVal, value);
            }
        }
    }

    if (minVal >= maxVal) {
        std::cerr << "Invalid min/max in file: " << filePath << "\n";
        return hist;
    }

    hist.minValue = minVal;
    hist.maxValue = maxVal;
    hist.binWidth = (maxVal - minVal) / PrecomputedHistogram::binCount;

    // --- Pass 2: Fill bins in parallel ---
    const unsigned int numThreads = std::thread::hardware_concurrency() - 1;
    std::vector<std::array<int, PrecomputedHistogram::binCount>> localBins(numThreads);

    std::vector<std::streampos> chunkStarts;
    std::ifstream file(filePath, std::ios::binary);

    // Determine approximate chunk boundaries
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    std::streampos chunkSize = fileSize / numThreads;
    chunkStarts.push_back(0);
    for (unsigned int i = 1; i < numThreads; ++i) {
        chunkStarts.push_back(i * chunkSize);
    }
    chunkStarts.push_back(fileSize); // end of file

    auto worker = [&](unsigned int threadIndex) {
        std::ifstream in(filePath);
        in.seekg(chunkStarts[threadIndex]);

        // Move to next line start if not the first chunk
        if (threadIndex != 0) {
            std::string dummy;
            std::getline(in, dummy);
        }

        double min = hist.minValue;
        double binW = hist.binWidth;
        std::array<int, PrecomputedHistogram::binCount>& bins = localBins[threadIndex];

        std::string line;
        while (in.tellg() < chunkStarts[threadIndex + 1] && std::getline(in, line)) {
            double value;
            if (parseFirstNumber(line, value)) {
                int bin = static_cast<int>((value - min) / binW);
                if (bin < 0) bin = 0;
                if (bin >= PrecomputedHistogram::binCount) bin = PrecomputedHistogram::binCount - 1;
                bins[bin]++;
            }
        }
    };

    // Launch threads
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker, i);
    }
    for (auto& t : threads) t.join();

    // Combine local bins
    for (auto& local : localBins) {
        for (int i = 0; i < PrecomputedHistogram::binCount; ++i) {
            hist.binCounts[i] += local[i];
        }
    }

    return hist;
}
