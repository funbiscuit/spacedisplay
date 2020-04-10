
#ifndef SPACEDISPLAY_FILEENTRYPOOL_H
#define SPACEDISPLAY_FILEENTRYPOOL_H

#include <unordered_map>
#include <memory>
#include "fileentry.h"

class FileEntry;

class FileEntryPool {

public:
    FileEntryPool();

    std::unique_ptr<FileEntry> create_entry(uint64_t id, const std::string& name, FileEntry::EntryType entryType);

    void cache_children(std::unique_ptr<FileEntry> firstChild);

    // these two functions are not very safe and using them a lot causes random crashes
    // so use only caching and let OS cleanup memory after us
    void delete_children(std::unique_ptr<FileEntry> firstChild);
    void cleanup_cache();

private:
    //pointers to entries in this cache are valid but their pointers should not be used
    //e.g. pointer to char array (entry name) might be used in some other entry, so it should be replaced
    std::vector<std::unique_ptr<FileEntry>> entriesCache;

    //map key is length of stored string (allocated memory is key+1)
    std::unordered_map<size_t, std::vector<std::unique_ptr<char[]>>> charsCache;

    uint64_t _cache_children(std::unique_ptr<FileEntry> firstChild);
    uint64_t _delete_children(std::unique_ptr<FileEntry> firstChild);

};
#endif //SPACEDISPLAY_FILEENTRYPOOL_H
