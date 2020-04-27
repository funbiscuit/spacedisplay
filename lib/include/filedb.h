#ifndef SPACEDISPLAY_FILEDB_H
#define SPACEDISPLAY_FILEDB_H

#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <unordered_map>

class FileEntry;
class FilePath;

/**
 * Implements tree structure for files/directories
 * Also can store information about total and available space
 */
class FileDB
{
public:
    /**
     * Sets space of mount point where files in this db are stored
     * @param totalSpace
     * @param availableSpace
     */
    void setSpace(uint64_t totalSpace, uint64_t availableSpace);

    /**
     * Tries to find entry by specified path and updates its children to be the same as in
     * provided path. All existing children that are not listed in this vector, will be deleted.
     * If path doesn't exist - entries are destroyed and false is returned
     * @param path
     * @param entries
     * @return
     */
    bool setChildrenForPath(const FilePath& path, std::vector<std::unique_ptr<FileEntry>> entries);

    /**
     * Clears database and sets new root, making db initialized
     * @param path
     */
    void setNewRootPath(const std::string& path);

    /**
     * Returns path to current root or null if db is not initialized
     * @return
     */
    const FilePath* getRootPath() const;

    /**
     * Clears database leaving it in uninitialized state
     * @param path
     */
    void clearDb();

    /**
     * Tries to find dir entry by given FilePath
     * If it was found - clears all children
     * @param path - path to entry that should be found
     * @param childrenCount - number of deleted children, 0 if none
     * @return pointer to entry if it was found, nullptr otherwise
     */
    const FileEntry* clearEntry(const FilePath& path, uint64_t& childrenCount);

    /**
     * Tries to find file/dir entry by given FilePath
     * @param path - path to entry that should be found
     * @return pointer to entry if it was found, nullptr otherwise
     */
    const FileEntry* findEntry(const FilePath& path) const;

    /**
     * Let's you access entry at arbitrary path
     * Data is safe to access only inside callback func, do not save it
     * Database is locked during processing so don't spend much time
     * By processing root db assumes, you read all changes so hasChanges is set to false
     * If db is not initialized, func will not be called and false is returned
     * @param func
     * @return false if db not initialized
     */
    bool processEntry(const FilePath& path, const std::function<void(const FileEntry&)>& func) const;

    /**
     * Checks if database is valid (i.e. has a valid root) and can be accessed
     * @return
     */
    bool isReady() const;

    bool hasChanges() const;

    void getSpace(uint64_t& used, uint64_t& available, uint64_t& total) const;

    uint64_t getFileCount() const;

private:

    uint64_t totalSpace = 0;
    uint64_t availableSpace = 0;

    mutable std::mutex dbMtx;

    std::atomic<bool> rootValid;
    std::atomic<uint64_t> usedSpace;
    std::atomic<uint64_t> fileCount;

    //TODO make it possible to access "has changes" info by some caller id
    mutable std::atomic<bool> bHasChanges;

    std::unique_ptr<FileEntry> rootFile;
    std::unique_ptr<FilePath> rootPath;

    //map key is crc of entry path, map value is vector of all children with the same crc of their name
    std::unordered_map<uint16_t, std::vector<FileEntry*>> entriesMap;

    void _clearDb();
    FileEntry* _findEntry(const FilePath& path) const;

    /**
     * Deletes all items from entriesMap for this entry and all children (recursively)
     * @param entry
     */
    void _cleanupEntryCrc(const FileEntry& entry);

};


#endif //SPACEDISPLAY_FILEDB_H
