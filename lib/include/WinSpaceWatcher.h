#ifndef SPACEDISPLAY_WINSPACEWATCHER_H
#define SPACEDISPLAY_WINSPACEWATCHER_H

#include "spacewatcher.h"

#include <string>
#include <memory>
#include <list>
#include <atomic>

#include <Windows.h>

class WinSpaceWatcher : public SpaceWatcher {
public:
    ~WinSpaceWatcher() override;

    int64_t getDirCountLimit() const override { return -1; }

    friend std::unique_ptr<SpaceWatcher> SpaceWatcher::create(const std::string &path);
protected:
    void readEvents() override;

private:
    WinSpaceWatcher();

    const int watchBufferSize = 1024 * 48;

    std::atomic<HANDLE> watchedDir;

    std::string watchedPath;

    std::unique_ptr<DWORD[]> watchBuffer;

    bool beginWatch(const std::string &path);
};

#endif //SPACEDISPLAY_WINSPACEWATCHER_H
