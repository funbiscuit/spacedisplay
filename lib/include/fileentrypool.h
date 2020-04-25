
#ifndef SPACEDISPLAY_FILEENTRYPOOL_H
#define SPACEDISPLAY_FILEENTRYPOOL_H

#include <unordered_map>
#include <memory>
#include <vector>
#include <mutex>

class FileEntry;

class FileEntryPool {

public:
    FileEntryPool();
    ~FileEntryPool();

    std::unique_ptr<FileEntry> create_entry(const std::string& name, bool isDir);

    int64_t cache_children(std::unique_ptr<FileEntry> firstChild);

    void cleanup_cache();

private:
    // cache could be accessed from different threads
    mutable std::mutex cacheMtx;

    //pointers to entries in this cache are valid but their pointers should not be used
    //e.g. pointer to char array (entry name) might be used in some other entry, so it should be replaced
    std::vector<std::unique_ptr<FileEntry>> entriesCache;

    //map key is length of stored string (allocated memory is key+1)
    std::unordered_map<size_t, std::vector<std::unique_ptr<char[]>>> charsCache;

    uint64_t _cache_children(std::unique_ptr<FileEntry> firstChild);

};
#endif //SPACEDISPLAY_FILEENTRYPOOL_H
