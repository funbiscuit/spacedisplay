
#ifndef SPACEDISPLAY_FILEENTRYPOOL_H
#define SPACEDISPLAY_FILEENTRYPOOL_H

#include <boost/pool/object_pool.hpp>
#include <unordered_map>
#include "fileentry.h"

class FileEntry;

class FileEntryPool {

public:
    FileEntryPool();

    FileEntry* create_entry(uint64_t id, const char *name, FileEntry::EntryType entryType);

    void destroy_entries();

    void async_destroy_children(FileEntry* firstChild);

private:
    //pointers to entries in this cache are valid bu their properties should not be used
    //e.g. pointer to char array (entry name) might be used in some other entry, so it should be replaced
    std::vector<FileEntry*> entriesCache;

    //map key is length of stored string (allocated memory is key+1)
    std::unordered_map<int, std::vector<char*>> charsCache;

    void destroy_children(FileEntry* firstChild);
    boost::pool<>* charPool;
    boost::object_pool<FileEntry>* entryPool;

};
#endif //SPACEDISPLAY_FILEENTRYPOOL_H
