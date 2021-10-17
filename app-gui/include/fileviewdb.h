#ifndef SPACEDISPLAY_FILEVIEWDB_H
#define SPACEDISPLAY_FILEVIEWDB_H

#include <memory>
#include <functional>
#include "utils.h"

class FileDB;

class FilePath;

class FileEntryView;

/**
 * Class for storing FileViewEntries, not thread-safe
 * so should be accessed only from GUI thread
 */
class FileViewDB {

public:
    /**
     * Updates file views with files from db. Returns true if anything changed
     * If any pointers were saved to db entries before update, they should be cleared
     * It is not safe to access any pre-update pointers after update!
     * Pointers are valid only until the next update
     * @param db - FileDB to use to access data
     * @param includeUnknown - whether to include file which represents unknown/unscanned space
     * @param includeAvailable - whether to include file which represents available/free space
     * @return
     */
    bool update(const FileDB &db, bool includeUnknown, bool includeAvailable);

    void onThemeChanged();

    void setViewArea(Utils::RectI rect);

    void setViewPath(const FilePath &filepath);

    void setViewDepth(int depth);

    /**
     * Used for calculating required height for file views
     * @param depth
     */
    void setTextHeight(int height);

    uint64_t getFilesSize() const;

    /**
     * Let's you access entry at process (display) it
     * If db is not initialized, func will not be called and false is returned
     * @param func
     * @return false if db not initialized
     */
    bool processEntry(const std::function<void(FileEntryView &)> &func);

    FileEntryView *getHoveredView(int mouseX, int mouseY);

    FileEntryView *getClosestView(const FilePath &filepath, int maxDepth);

private:

    std::shared_ptr<FileEntryView> rootFile;

    std::unique_ptr<FilePath> viewPath;
    Utils::RectI viewRect;
    int viewDepth;
    int textHeight;

};


#endif //SPACEDISPLAY_FILEVIEWDB_H
