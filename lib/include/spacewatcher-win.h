#ifndef SPACEDISPLAY_SPACEWATCHER_WIN_H
#define SPACEDISPLAY_SPACEWATCHER_WIN_H

#include "spacewatcher.h"

#include <string>
#include <memory>
#include <list>
#include <atomic>

#include <Windows.h>

class SpaceWatcherWin : public SpaceWatcher {
public:

    SpaceWatcherWin();
    ~SpaceWatcherWin() override;

    bool beginWatch(const std::string& path) override;
    void endWatch() override;
    bool isWatching() override;


    void addDir(const std::string& path) override {}

protected:
    void readEvents() override;

private:

    const int watchBufferSize = 1024*48;

    std::atomic<HANDLE> watchedDir;

    std::unique_ptr<DWORD[]> watchBuffer;

    void _endWatch();
};

#endif //SPACEDISPLAY_SPACEWATCHER_WIN_H
