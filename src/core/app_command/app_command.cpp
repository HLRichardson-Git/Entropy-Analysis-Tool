
#include "app_command.h"

void CommandQueue::Push(const AppCommand& cmd) {
    queue.push(cmd);
}

bool CommandQueue::Pop(AppCommand& out) {
    if (queue.empty()) return false;
    out = queue.front();
    queue.pop();
    return true;
}