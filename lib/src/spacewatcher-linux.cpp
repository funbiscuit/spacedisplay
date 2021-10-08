#include "spacewatcher-linux.h"
#include "utils.h"

#include <sys/inotify.h>
#include <unistd.h>

#include <iostream>
#include <fstream>

SpaceWatcherLinux::SpaceWatcherLinux() : inotifyFd(-1), watchBuffer(watchBufferSize, 0) {
    startThread();
}

SpaceWatcherLinux::~SpaceWatcherLinux() {
    // endWatch() is virtual so we can't call it in destructor
    _endWatch();
    stopThread();
}

bool SpaceWatcherLinux::beginWatch(const std::string &path) {
    endWatch();

    inotifyFd = inotify_init1(IN_NONBLOCK);
    if (inotifyFd != -1)
        return addDir(path) == SpaceWatcher::AddDirStatus::ADDED;
    return false;
}

void SpaceWatcherLinux::endWatch() {
    _endWatch();
}

void SpaceWatcherLinux::_endWatch() {
    if (inotifyFd != -1) {
        close(inotifyFd);
        inotifyFd = -1;
        std::lock_guard<std::mutex> lock(inotifyWdsMtx);
        inotifyWds.clear();
    }
}

bool SpaceWatcherLinux::isWatching() const {
    return inotifyFd != -1;
}

int64_t SpaceWatcherLinux::getDirCountLimit() const {
    int64_t limit = 0;

    std::ifstream file("/proc/sys/fs/inotify/max_user_watches");
    file >> limit;

    return limit;
}

SpaceWatcher::AddDirStatus SpaceWatcherLinux::addDir(const std::string &path) {
    if (inotifyFd == -1)
        return AddDirStatus::NOT_INITIALIZED;

    int wd;

    //not using IN_DELETE_SELF and IN_MOVE_SELF since this events should be detected by parent directory
    const auto EVENTS = IN_MODIFY | IN_MOVE | IN_CREATE | IN_DELETE | IN_MODIFY;

    wd = inotify_add_watch(inotifyFd, path.c_str(), EVENTS);
    //TODO we should detect when process runs out of available inotify watches
    // so client could notify user about it
    if (wd == -1) {
        switch (errno) {
            case ENOSPC:
                return AddDirStatus::DIR_LIMIT_REACHED;
            case EBADF:
                return AddDirStatus::NOT_INITIALIZED;
        }
        return AddDirStatus::ACCESS_DENIED; //this is a default, although not very accurate
    }
    std::lock_guard<std::mutex> lock(inotifyWdsMtx);
    auto it = inotifyWds.find(wd);
    if (it != inotifyWds.end())
        it->second = path;
    else {
        inotifyWds[wd] = path;
        SpaceWatcher::addDir(path);
    }
    return AddDirStatus::ADDED;
}

void SpaceWatcherLinux::rmDir(const std::string &path) {
    //TODO
    SpaceWatcher::rmDir(path);
}

void SpaceWatcherLinux::_addEvent(struct inotify_event *inotifyEvent) {
    if (inotifyEvent->mask & IN_IGNORED) {
        //watch was removed so remove it from our map
        std::lock_guard<std::mutex> lock(inotifyWdsMtx);
        auto it = inotifyWds.find(inotifyEvent->wd);
        if (it != inotifyWds.end()) {
            SpaceWatcher::rmDir(it->second);
            inotifyWds.erase(it);
        }
        return;
    }

    if (inotifyEvent->len == 0)
        return;

    auto fileEvent = Utils::make_unique<FileEvent>();

    if (inotifyEvent->mask & IN_CREATE)
        fileEvent->action = FileAction::ADDED;
    else if (inotifyEvent->mask & IN_DELETE)
        fileEvent->action = FileAction::REMOVED;
    else if (inotifyEvent->mask & IN_MODIFY)
        fileEvent->action = FileAction::MODIFIED;
    else if (inotifyEvent->mask & IN_MOVED_FROM)
        fileEvent->action = FileAction::OLD_NAME;
    else if (inotifyEvent->mask & IN_MOVED_TO)
        fileEvent->action = FileAction::NEW_NAME;
    else
        return;

    {
        std::lock_guard<std::mutex> lock(inotifyWdsMtx);

        auto it = inotifyWds.find(inotifyEvent->wd);
        if (it == inotifyWds.end())
            return; //should not happen

        fileEvent->filepath = it->second + inotifyEvent->name;
    }

    if (fileEvent->filepath.back() == '/')
        fileEvent->filepath.pop_back();

    auto lastSlash = fileEvent->filepath.find_last_of('/');
    if (lastSlash != std::string::npos)
        fileEvent->parentpath = fileEvent->filepath.substr(0, lastSlash + 1);
    else
        fileEvent->parentpath = "";

    addEvent(std::move(fileEvent));
}

void SpaceWatcherLinux::readEvents() {
    if (!isWatching())
        return;

    while (true) {
        auto numRead = read(inotifyFd, watchBuffer.data(), watchBufferSize);
        if (numRead <= 0)
            break;

        for (size_t i = 0; i < numRead;) {
            auto event = reinterpret_cast<inotify_event *>(&watchBuffer[i]);
            _addEvent(event);

            i += sizeof(inotify_event) + event->len;
        }
    }
}
