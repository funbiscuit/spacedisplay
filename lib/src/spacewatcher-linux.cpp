#include "spacewatcher-linux.h"
#include "utils.h"

#include <sys/inotify.h>
#include <unistd.h>

#include <iostream>

SpaceWatcherLinux::SpaceWatcherLinux() : inotifyFd(-1), watchBuffer(watchBufferSize, 0)
{
    startThread();
}

SpaceWatcherLinux::~SpaceWatcherLinux()
{
    // endWatch() is virtual so we can't call it in destructor
    _endWatch();
    stopThread();
}

bool SpaceWatcherLinux::beginWatch(const std::string& path)
{
    endWatch();

    inotifyFd = inotify_init1(IN_NONBLOCK);
    if(inotifyFd != -1)
        return addDir(path);
    return false;
}

void SpaceWatcherLinux::endWatch()
{
    _endWatch();
}

void SpaceWatcherLinux::_endWatch()
{
    if(inotifyFd != -1)
    {
        close(inotifyFd);
        inotifyFd = -1;
        std::lock_guard<std::mutex> lock(inotifyWdsMtx);
        inotifyWds.clear();
    }
}

bool SpaceWatcherLinux::isWatching()
{
    return inotifyFd != -1;
}

bool SpaceWatcherLinux::addDir(const std::string &path)
{
    if(inotifyFd == -1)
        return false;

    int wd;

    //not using IN_DELETE_SELF and IN_MOVE_SELF since this events should be detected by parent directory
    const auto EVENTS = IN_MODIFY | IN_MOVE | IN_CREATE | IN_DELETE | IN_MODIFY;

    wd = inotify_add_watch(inotifyFd, path.c_str(), EVENTS);
    //TODO we should detect when process runs out of available inotify watches
    // so client could notify user about it
    if (wd == -1)
        return false;
    std::lock_guard<std::mutex> lock(inotifyWdsMtx);
    inotifyWds[wd]=path;
    return true;
}

void SpaceWatcherLinux::rmDir(const std::string &path)
{
    //TODO
}

void SpaceWatcherLinux::_addEvent(struct inotify_event *inotifyEvent)
{
    if ((inotifyEvent->mask & IN_IGNORED) || inotifyEvent->len == 0)
        return;

    auto fileEvent = Utils::make_unique<FileEvent>();

    if(inotifyEvent->mask & IN_CREATE)
        fileEvent->action = FileAction::ADDED;
    else if(inotifyEvent->mask & IN_DELETE)
        fileEvent->action = FileAction::REMOVED;
    else if(inotifyEvent->mask & IN_MODIFY)
        fileEvent->action = FileAction::MODIFIED;
    else if(inotifyEvent->mask & IN_MOVED_FROM)
        fileEvent->action = FileAction::OLD_NAME;
    else if(inotifyEvent->mask & IN_MOVED_TO)
        fileEvent->action = FileAction::NEW_NAME;
    else
        return;

    {
        std::lock_guard<std::mutex> lock(inotifyWdsMtx);

        auto it = inotifyWds.find(inotifyEvent->wd);
        if(it == inotifyWds.end())
            return; //should not happen

        fileEvent->filepath = it->second + inotifyEvent->name;
    }

    if(fileEvent->filepath.back() == '/')
        fileEvent->filepath.pop_back();

    auto lastSlash = fileEvent->filepath.find_last_of('/');
    if(lastSlash != std::string::npos)
        fileEvent->parentpath = fileEvent->filepath.substr(0, lastSlash+1);
    else
        fileEvent->parentpath = "";

    addEvent(std::move(fileEvent));
}

void SpaceWatcherLinux::readEvents()
{
    if(!isWatching())
        return;

    while(true)
    {
        auto numRead = read(inotifyFd, watchBuffer.data(), watchBufferSize);
        if (numRead <= 0)
            break;

        for(size_t i=0; i<numRead; )
        {
            auto event = reinterpret_cast<inotify_event*>(&watchBuffer[i]);
            _addEvent(event);

            i += sizeof(inotify_event) + event->len;
        }
    }
}
