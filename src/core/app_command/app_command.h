
#pragma once

#include <string>
#include <variant>
#include <queue>
#include <filesystem>

#include "../types.h"
#include <lib90b/non_iid.h>

struct OpenProjectCommand {
    std::string filePath;
};

struct SaveProjectCommand {
    // Nothing for now since just saving currently loaded project
};

struct NewProjectCommand {
    std::string vendor;
    std::string repo;
    std::string projectName;
};

struct AddOECommand {
    std::string oeName;
};

struct DeleteOECommand {
    int oeIndex;
};

struct ProcessHistogramCommand {
    int oeIndex;
};

struct RunNonIidTestCommand {
    std::filesystem::path inputFile;
    std::filesystem::path*outputFile;
    std::string* result;
    NonIidParsedResults* nonIidParsedResults;

    TestTimer* testTimer;
};

struct RunRestartTestCommand {
    double minEntropy;
    std::filesystem::path inputFile;
    std::filesystem::path* outputFile;
    std::string* result;

    TestTimer* testTimer;
};

struct FindPassingDecimationCommand {
    int oeIndex;

    std::filesystem::path inputFile;
    std::shared_ptr<std::string> output;

    TestTimer* testTimer;
};

using AppCommand = std::variant<
    OpenProjectCommand,
    SaveProjectCommand,
    NewProjectCommand,
    AddOECommand,
    DeleteOECommand,
    ProcessHistogramCommand,
    RunNonIidTestCommand,
    RunRestartTestCommand,
    FindPassingDecimationCommand
>;

class CommandQueue {
private:
    std::queue<AppCommand> queue;

public:
    void Push(const AppCommand& cmd);
    bool Pop(AppCommand& out);

    bool Empty() const { return queue.empty(); }
};
