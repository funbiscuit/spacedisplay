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
        ADDED = 1,
        REMOVED = -1,
        MODIFIED = 3,
        OLD_NAME = -6,
        NEW_NAME = 6
    };

    enum class AddDirStatus {
        ADDED,
        DIR_LIMIT_REACHED, //used when unable to add inotify watch in linux
        ACCESS_DENIED,
    };

    struct FileEvent {
        FileAction action;
        std::string filepath;
        std::string parentpath;
    };

    /**
     * Create SpaceWatcher instance and start watching specific path
     * If platform doesn't support recursive watching, only this
     * directory will be watched. Add more with calls to addDir
     * @param path
     * @throws std::runtime_error if failed to start watching provided path
     * @return
     */
    static std::unique_ptr<SpaceWatcher> create(const std::string &path);

    virtual ~SpaceWatcher();

    /**
     * Get a limit for a number of watched directories.
     * Windows doesn't have such limit so -1 is returned.
     * Linux is limited by user limit of inotify watch descriptors.
     * @return limit of watched directories, -1 if there is no such limit.
     */
    virtual int64_t getDirCountLimit() const = 0;

    /**
     * Get number of directories that are added to watch using addDir();
     * If this watcher doesn't have a limit on watched dirs, this will always
     * return 1
     * @return
     */
    int64_t getWatchedDirCount() const;

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
    virtual AddDirStatus addDir(const std::string &path);

    /**
     * Adds directory to watch. Watch should be already started.
     * Used on platforms that can't watch recursively (e.g. linux)
     * @param path
     */
    virtual void rmDir(const std::string &path);

protected:
    SpaceWatcher();

    virtual void readEvents() = 0;


    void addEvent(std::unique_ptr<FileEvent> event);

private:

    int64_t watchedDirCount;

    std::mutex eventsMtx;
    std::list<std::unique_ptr<FileEvent>> eventQueue;


    std::atomic<bool> runThread;
    std::thread watchThread;

    void watcherRun();
};


#endif //SPACEDISPLAY_SPACEWATCHER_H
