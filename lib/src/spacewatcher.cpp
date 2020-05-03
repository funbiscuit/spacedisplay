#include "spacewatcher.h"

#ifdef _WIN32
#include "spacewatcher-win.h"
#else
#include "spacewatcher-linux.h"
#endif

#include <iostream>

SpaceWatcher::SpaceWatcher() : runThread(false)
{

}

SpaceWatcher::~SpaceWatcher()
{
}

void SpaceWatcher::startThread()
{
    if(runThread)
        return;
    runThread = true;
    //Start thread after everything is initialized
    watchThread=std::thread(&SpaceWatcher::watcherRun, this);
}

void SpaceWatcher::stopThread()
{
    if(!runThread)
        return;

    runThread = false;
    watchThread.join();
}

std::unique_ptr<SpaceWatcher::FileEvent> SpaceWatcher::popEvent()
{
    std::lock_guard<std::mutex> lock(eventsMtx);
    if(!eventQueue.empty())
    {
        auto event = std::move(eventQueue.front());
        eventQueue.pop_front();
        return event;
    }
    return nullptr;
}

void SpaceWatcher::watcherRun()
{
    std::cout << "Start watcher thread\n";
    while(runThread)
    {
        if(isWatching())
            readEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    std::cout << "Stop watcher thread\n";
}

std::unique_ptr<SpaceWatcher> SpaceWatcher::getWatcher()
{
#ifdef _WIN32
    return std::unique_ptr<SpaceWatcher>(new SpaceWatcherWin());
#else
    return std::unique_ptr<SpaceWatcher>(new SpaceWatcherLinux());
#endif
}

void SpaceWatcher::addEvent(std::unique_ptr<FileEvent> event)
{
    if(!event)
        return;

    std::lock_guard<std::mutex> lock(eventsMtx);
    //TODO check if we already have the same events in queue
    eventQueue.push_back(std::move(event));
}
