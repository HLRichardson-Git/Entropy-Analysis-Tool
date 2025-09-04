
#pragma once

#include <string>
#include <variant>
#include <queue>

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

using AppCommand = std::variant<
    OpenProjectCommand,
    SaveProjectCommand,
    NewProjectCommand,
    AddOECommand,
    DeleteOECommand
>;

class CommandQueue {
private:
    std::queue<AppCommand> queue;

public:
    void Push(const AppCommand& cmd);
    bool Pop(AppCommand& out);

    bool Empty() const { return queue.empty(); }
};
