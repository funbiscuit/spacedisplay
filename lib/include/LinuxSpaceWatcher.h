#ifndef SPACEDISPLAY_LINUXSPACEWATCHER_H
#define SPACEDISPLAY_LINUXSPACEWATCHER_H

#include "spacewatcher.h"

#include <string>
#include <memory>
#include <atomic>
#include <vector>
#include <unordered_map>

struct inotify_event;

class LinuxSpaceWatcher : public SpaceWatcher {
public:
    ~LinuxSpaceWatcher() override;

    int64_t getDirCountLimit() const override;

    AddDirStatus addDir(const std::string &path) override;

    void rmDir(const std::string &path) override;

    friend std::unique_ptr<SpaceWatcher> SpaceWatcher::create(const std::string &path);
protected:
    void readEvents() override;

private:

    const int watchBufferSize = 1024 * 48;

    std::vector<uint8_t> watchBuffer;

    int inotifyFd;
    std::unordered_map<int, std::string> inotifyWds;
    std::mutex inotifyWdsMtx;

    LinuxSpaceWatcher();

    bool beginWatch(const std::string &path);

    void processInotifyEvent(struct inotify_event *inotifyEvent);
};

#endif //SPACEDISPLAY_LINUXSPACEWATCHER_H
