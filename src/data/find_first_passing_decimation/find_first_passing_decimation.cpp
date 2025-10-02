
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>

#include "../../file_utils/file_utils.h"

std::string parsePerlOutputString(const std::string& output) {
    std::istringstream stream(output);
    std::string line;
    bool foundRate = false;
    int rate = 0;
    bool noDecimationRequired = false;
    bool maxDecimationFails = false;
    bool moreDataRequired = false;

    while (std::getline(stream, line)) {
        if (line.find("The first passing decimation rate is") != std::string::npos) {
            size_t pos = line.find("The first passing decimation rate is");
            if (pos != std::string::npos) {
                std::string sub = line.substr(pos + 35);
                std::istringstream numberStream(sub);
                numberStream >> rate;
                if (numberStream) {
                    foundRate = true;
                }
            }
        } else if (line.find("Invariant failed: No decimation passes IID testing. No decimation required to test using the essentially IID approach.") != std::string::npos) {
            noDecimationRequired = true;
        } else if (line.find("Invariant failed: Max decimation fails IID testing") != std::string::npos) {
            maxDecimationFails = true;
        } else if (line.find("More data is required to test with the provided decimation level.") != std::string::npos) {
            moreDataRequired = true;
        }
    }

    if (foundRate) {
        return "Passed: Found passing decimation rate: " + std::to_string(rate);
    } else if (noDecimationRequired) {
        return "Failed: No decimation required to test using the essentially IID approach.";
    } else if (maxDecimationFails) {
        return "Failed: Proceed with the non-decimated data using empirical approach.";
    } else if (moreDataRequired) {
        return "Error: More data is required to test with the provided decimation level.";
    } else {
        return "Decimation result could not be determined";
    }
}

std::string findFirstPassingDecimation(const std::filesystem::path& filepath) {
    std::string linuxPath = toWslCommandPath(filepath);
    std::string cmd = "wsl perl /home/user/tools/find-first-passing-decimation.pl " + linuxPath + " 24 2>&1";

    std::string output = executeCommand(cmd);
    std::string resultString = parsePerlOutputString(output);
    
    std::filesystem::path logFile = filepath.parent_path() / "firstPassingDecimationResult.txt";
    writeStringToFile(output, logFile);

    return resultString;
}