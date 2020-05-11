#include "logger.h"

#include "utils.h"
#include <iostream>



Logger::Logger() : hasNewLog(false)
{

}

void Logger::log(const std::string &msg, const std::string &tag) {
    std::lock_guard<std::mutex> lock(logMtx);

    history.emplace_back(Utils::strFormat("[%s] %s", tag.c_str(), msg.c_str()));
    std::cout << history.back() << std::endl;
    hasNewLog = true;
}

std::vector<std::string> Logger::getHistory() {
    std::lock_guard<std::mutex> lock(logMtx);
    hasNewLog = false;
    return history;
}

void Logger::clear() {
    std::lock_guard<std::mutex> lock(logMtx);
    history.clear();
    hasNewLog = false;
}

bool Logger::hasNew() {
    //TODO should add option to remember multiple clients that can
    // have different states of `hasNew` property
    return hasNewLog;
}
