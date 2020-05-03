#ifndef SPACEDISPLAY_SPACEWATCHER_H
#define SPACEDISPLAY_SPACEWATCHER_H

#include <string>
#include <memory>
#include <mutex>
#include <list>
#include <thread>
#include <atomic>


class SpaceWatcher {
public:
    enum class FileAction {
        ADDED=1,
        REMOVED=-1,
        MODIFIED=3,
        OLD_NAME=-6,
        NEW_NAME=6
    };

    struct FileEvent {
        FileAction action;
        std::string filepath;
        std::string parentpath;
    };

    static std::unique_ptr<SpaceWatcher> getWatcher();

    SpaceWatcher();
    virtual ~SpaceWatcher();

    virtual bool beginWatch(const std::string& path) = 0;
    virtual void endWatch() = 0;
    virtual bool isWatching() = 0;

    /**
     * Returns event if any is present.
     * @return nullptr if there are no events
     */
    std::unique_ptr<FileEvent> popEvent();

    /**
     * Adds directory to watch. Watch should be already started.
     * Used on platforms that can't watch recursively (e.g. linux)
     * @param path
     */
    virtual void addDir(const std::string& path) = 0;

    /**
     * Adds directory to watch. Watch should be already started.
     * Used on platforms that can't watch recursively (e.g. linux)
     * @param path
     */
    virtual void rmDir(const std::string& path) = 0;

protected:

    std::string watchedPath;

    virtual void readEvents() = 0;

    // start and stop thread are called from child classes
    void startThread();
    void stopThread();

    void addEvent(std::unique_ptr<FileEvent> event);
private:

    std::mutex eventsMtx;
    std::list<std::unique_ptr<FileEvent>> eventQueue;


    std::atomic<bool> runThread;
    std::thread watchThread;

    void watcherRun();
};




#endif //SPACEDISPLAY_SPACEWATCHER_H
