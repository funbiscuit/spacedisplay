#include "spacewatcher.h"

#include <iostream>

SpaceWatcher::~SpaceWatcher() {
    runThread = false;
    watchThread.join();
}

int64_t SpaceWatcher::getWatchedDirCount() const {
    return getDirCountLimit() < 0 ? 1 : watchedDirCount;
}

SpaceWatcher::AddDirStatus SpaceWatcher::addDir(const std::string &path) {
    ++watchedDirCount;
    return AddDirStatus::ADDED;
}

void SpaceWatcher::rmDir(const std::string &path) {
    if (watchedDirCount > 0)
        --watchedDirCount;
}

std::unique_ptr<SpaceWatcher::FileEvent> SpaceWatcher::popEvent() {
    std::lock_guard<std::mutex> lock(eventsMtx);
    if (!eventQueue.empty()) {
        auto event = std::move(eventQueue.front());
        eventQueue.pop_front();
        return event;
    }
    return nullptr;
}

SpaceWatcher::SpaceWatcher() : runThread(true), watchedDirCount(0) {
    //Start thread after everything is initialized
    watchThread = std::thread(&SpaceWatcher::watcherRun, this);
}

void SpaceWatcher::addEvent(std::unique_ptr<FileEvent> event) {
    if (!event)
        return;

    std::lock_guard<std::mutex> lock(eventsMtx);
    //TODO check if we already have the same events in queue
    eventQueue.push_back(std::move(event));
}

void SpaceWatcher::watcherRun() {
    std::cout << "Start watcher thread\n";
    while (runThread) {
        readEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    std::cout << "Stop watcher thread\n";
}
