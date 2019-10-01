
#ifndef SPACEDISPLAY_FILEENTRYPOOL_H
#define SPACEDISPLAY_FILEENTRYPOOL_H

#include <unordered_map>
#include "fileentry.h"

class FileEntry;

class FileEntryPool {

public:
    FileEntryPool();

    FileEntry* create_entry(uint64_t id, const char *name, FileEntry::EntryType entryType);

    void cache_children(FileEntry* firstChild);

    // these two functions are not very safe and using them a lot causes random crashes
    // so use only caching and let OS cleanup memory after us
    void delete_children(FileEntry* firstChild);
    void cleanup_cache();

private:
    //pointers to entries in this cache are valid but their pointers should not be used
    //e.g. pointer to char array (entry name) might be used in some other entry, so it should be replaced
    std::vector<FileEntry*> entriesCache;

    //map key is length of stored string (allocated memory is key+1)
    std::unordered_map<int, std::vector<char*>> charsCache;

    uint64_t _cache_children(FileEntry* firstChild);
    uint64_t _delete_children(FileEntry* firstChild);

};
#endif //SPACEDISPLAY_FILEENTRYPOOL_H
