
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdexcept>

#include <lib90b/iid.h>

const int MIN_SAMPLES = 1'000'000;

// Compute max decimation k = largest integer s.t. fileSize / k >= MIN_SAMPLES
static int maxDecimation(const std::filesystem::path& filepath) {
    if (!std::filesystem::exists(filepath))
        throw std::runtime_error("File does not exist: " + filepath.string());

    auto filesize = std::filesystem::file_size(filepath);
    if (filesize < MIN_SAMPLES)
        throw std::runtime_error("File too small: need at least 1,000,000 samples");

    return static_cast<int>(filesize / MIN_SAMPLES);
}

// Read the file into memory as bytes
static std::vector<uint8_t> readFileBytes(const std::filesystem::path& filepath) {
    std::ifstream inFile(filepath, std::ios::binary);
    if (!inFile)
        throw std::runtime_error("Cannot open file: " + filepath.string());

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(inFile)),
                               std::istreambuf_iterator<char>());

    if (data.size() < MIN_SAMPLES)
        throw std::runtime_error("File too small: need at least 1,000,000 samples");

    return data;
}

// Decimate in-memory vector: take every k-th element
static std::vector<uint8_t> decimateSamples(const std::vector<uint8_t>& data, int k) {
    size_t n = (data.size() + k - 1) / k;
    std::vector<uint8_t> decimated;
    decimated.reserve(n);

    const uint8_t* src = data.data();
    for (size_t i = 0; i < n; ++i) {
        decimated.push_back(src[i * k]);
    }
    return decimated;
}

#include <chrono>
#include <unordered_set>

// Run IID test on a vector of samples
static bool runIIDTest(const std::vector<uint8_t>& samples) {
    std::cout << "  Actual sample count being tested: " << samples.size() << "\n";
    std::cout << "  Memory size: " << (samples.size() * sizeof(uint8_t)) << " bytes\n";
    
    // Check for data characteristics
    std::unordered_set<uint8_t> unique_vals(samples.begin(), samples.end());
    std::cout << "  Unique values in sample: " << unique_vals.size() << "\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    lib90b::EntropyInputData data{samples, 8};
    
    std::cout << "  EntropyInputData created, starting test...\n";
    std::cout.flush();
    
    lib90b::IidResult result = lib90b::iidTestSuite(data);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    
    std::cout << "  Test completed in " << duration.count() << " seconds\n";
    
    return result.passed;
}

// Main workflow
bool findFirstPassingDecimation(const std::filesystem::path& filepath) {
    std::cout << "Using file: " << filepath << "\n";

    auto allSamples = readFileBytes(filepath);
    int maxK = maxDecimation(filepath);

    std::cout << "Total samples (bytes): " << allSamples.size() << "\n";
    std::cout << "Max decimation value (k): " << maxK << "\n";

    // Early return: test max decimation first
    std::cout << "Testing max decimation k=" << maxK << "...\n";
    auto decMax = decimateSamples(allSamples, maxK);
    std::cout << "Decimated sample count: " << decMax.size() << "\n";

    if (!runIIDTest(decMax)) {
        std::cout << "Max decimation k=" << maxK << " fails IID tests. No lower k will be tested.\n";
        return false;
    }
    std::cout << "Done" << std::endl;

    // Test lower decimations if needed
    for (int k = 1; k < maxK; ++k) {
        std::cout << "Testing decimation k=" << k << "...\n";
        auto dec = decimateSamples(allSamples, k);
        std::cout << "Decimated sample count: " << dec.size() << "\n";
        if (runIIDTest(dec)) {
            std::cout << "First passing decimation: k=" << k
                      << " (sample count: " << dec.size() << ")\n";
            return true;
        }
    }

    std::cout << "All lower decimations fail. Max decimation k=" << maxK
              << " is the first passing.\n";
    return true;
}



/*#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <unordered_set>
#include <chrono>

#include <lib90b/iid.h>

namespace fs = std::filesystem;

const int MIN_SAMPLES = 1'000'000;

// ----------------------------
// Utilities
// ----------------------------

// Compute max decimation k = largest integer s.t. fileSize / k >= MIN_SAMPLES
static int maxDecimation(const fs::path& filepath) {
    if (!fs::exists(filepath))
        throw std::runtime_error("File does not exist: " + filepath.string());

    auto filesize = fs::file_size(filepath);
    if (filesize < MIN_SAMPLES)
        throw std::runtime_error("File too small: need at least 1,000,000 samples");

    return static_cast<int>(filesize / MIN_SAMPLES);
}

// Read the file into memory as bytes
static std::vector<uint8_t> readFileBytes(const fs::path& filepath) {
    std::ifstream inFile(filepath, std::ios::binary);
    if (!inFile)
        throw std::runtime_error("Cannot open file: " + filepath.string());

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(inFile)),
                               std::istreambuf_iterator<char>());

    if (data.size() < MIN_SAMPLES)
        throw std::runtime_error("File too small: need at least 1,000,000 samples");

    return data;
}

// Decimate in-memory vector: take every k-th element
static std::vector<uint8_t> decimateSamples(const std::vector<uint8_t>& data, int k) {
    size_t n = (data.size() + k - 1) / k;
    std::vector<uint8_t> decimated;
    decimated.reserve(n);

    const uint8_t* src = data.data();
    for (size_t i = 0; i < n; ++i) {
        decimated.push_back(src[i * k]);
    }
    return decimated;
}

// Save decimated samples to a file
static fs::path saveDecimatedFile(const fs::path& inputFile, 
                                  const std::vector<uint8_t>& decimated, 
                                  int k) 
{
    fs::path dir = inputFile.parent_path();
    std::string stem = inputFile.stem().string();
    std::string decFilename = stem + "_decimated_k" + std::to_string(k) + ".bin";
    fs::path decPath = dir / decFilename;

    std::ofstream out(decPath, std::ios::binary);
    if (!out)
        throw std::runtime_error("Failed to write decimated file: " + decPath.string());

    out.write(reinterpret_cast<const char*>(decimated.data()), decimated.size());
    return decPath;
}

// Run IID test on a decimated file
static bool runIIDTestOnFile(const fs::path& filePath) {
    std::cout << "Running IID test on file: " << filePath << "\n";

    auto start = std::chrono::high_resolution_clock::now();
    lib90b::IidResult result = lib90b::iidTestSuite(filePath);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Test completed in " << duration.count() << " seconds\n";

    return result.passed;
}

// ----------------------------
// Main workflow
// ----------------------------
bool findFirstPassingDecimation(const fs::path& filepath) {
    std::cout << "Using file: " << filepath << "\n";

    auto allSamples = readFileBytes(filepath);
    int maxK = maxDecimation(filepath);

    std::cout << "Total samples: " << allSamples.size() << "\n";
    std::cout << "Max decimation k: " << maxK << "\n";

    // First test max decimation
    std::cout << "Testing max decimation k=" << maxK << "...\n";
    auto decMax = decimateSamples(allSamples, maxK);
    auto decFileMax = saveDecimatedFile(filepath, decMax, maxK);

    if (!runIIDTestOnFile(decFileMax)) {
        std::cout << "Max decimation k=" << maxK << " fails IID tests. No lower k will be tested.\n";
        return true; // return empty path
    }

    // Test lower decimations
    for (int k = 1; k < maxK; ++k) {
        std::cout << "Testing decimation k=" << k << "...\n";
        auto dec = decimateSamples(allSamples, k);
        auto decFile = saveDecimatedFile(filepath, dec, k);

        if (runIIDTestOnFile(decFile)) {
            std::cout << "First passing decimation: k=" << k 
                      << " -> saved at: " << decFile << "\n";
            return true;
        }
    }

    std::cout << "All lower decimations fail. Max decimation k=" << maxK 
              << " is the first passing -> saved at: " << decFileMax << "\n";

    return true;
}*/
