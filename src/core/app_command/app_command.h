
#pragma once

#include <string>
#include <variant>
#include <queue>
#include <filesystem>

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

struct RunStatisticalTestCommand {
    std::filesystem::path inputFile;   
    std::shared_ptr<lib90b::NonIidResult> output;

    std::optional<double> minValue;
    std::optional<double> maxValue;
    int regionIndex = 0;
};

using AppCommand = std::variant<
    OpenProjectCommand,
    SaveProjectCommand,
    NewProjectCommand,
    AddOECommand,
    DeleteOECommand,
    ProcessHistogramCommand,
    RunStatisticalTestCommand
>;

class CommandQueue {
private:
    std::queue<AppCommand> queue;

public:
    void Push(const AppCommand& cmd);
    bool Pop(AppCommand& out);

    bool Empty() const { return queue.empty(); }
};
