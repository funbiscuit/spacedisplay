#ifndef SPACEDISPLAY_SPACEWATCHER_H
#define SPACEDISPLAY_SPACEWATCHER_H

#include <string>

class SpaceWatcher {
public:
    enum class FileAction {
        ADDED=1,
        REMOVED=-1,
        MODIFIED=3,
        OLD_NAME=-6,
        NEW_NAME=6
    };

    struct FileInfo {
        FileAction Action;
        std::string FileName;
    };

    SpaceWatcher();
    ~SpaceWatcher();

    void watchDir(const std::string& path);

};




#endif //SPACEDISPLAY_SPACEWATCHER_H
