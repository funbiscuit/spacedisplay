#ifndef SPACEDISPLAY_SPACEWATCHER_LINUX_H
#define SPACEDISPLAY_SPACEWATCHER_LINUX_H

#include "spacewatcher.h"

#include <string>
#include <memory>
#include <atomic>
#include <vector>
#include <unordered_map>

struct inotify_event;

class SpaceWatcherLinux : public SpaceWatcher {
public:

    SpaceWatcherLinux();

    ~SpaceWatcherLinux() override;

    bool beginWatch(const std::string &path) override;

    void endWatch() override;

    bool isWatching() const override;

    int64_t getDirCountLimit() const override;

    AddDirStatus addDir(const std::string &path) override;

    void rmDir(const std::string &path) override;

protected:
    void readEvents() override;

private:

    const int watchBufferSize = 1024 * 48;

    std::vector<uint8_t> watchBuffer;

    int inotifyFd;
    std::unordered_map<int, std::string> inotifyWds;
    std::mutex inotifyWdsMtx;

    void _endWatch();

    void _addEvent(struct inotify_event *inotifyEvent);
};

#endif //SPACEDISPLAY_SPACEWATCHER_LINUX_H
