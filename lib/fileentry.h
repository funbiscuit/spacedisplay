
#ifndef SPACEDISPLAY_FILEENTRY_H
#define SPACEDISPLAY_FILEENTRY_H


#include <string>
#include <vector>
#include <memory>

class FileEntryPool;
class FilePath;

class FileEntry {

    FileEntry(std::unique_ptr<char[]> name_, bool isDir_);
public:

    void reconstruct(std::unique_ptr<char[]> name_, bool isDir_);

    static std::unique_ptr<FileEntry> createEntry(const std::string& name_, bool isDir_);

    /**
     * Deletes (or caches) all entries (and their children) in chain
     * entry->nextEntry->nextEntry and so on
     * @param firstEntry
     * @@return number of deleted (or cached) entries
     */
    static int64_t deleteEntryChain(std::unique_ptr<FileEntry> firstEntry);

    void set_size(int64_t size);

    void getPath(FilePath& _path);

    int64_t get_size() const;

    const char* getName() const;

    const FileEntry* getFirstChild() const;

    const FileEntry* getNext() const;

    uint16_t getNameCrc16() const;

    /**
     * Adds child to children of this entry.
     * Adds to relevant place so all children are sorted by size (in decreasing order)
     * @param child
     */
    void add_child(std::unique_ptr<FileEntry> child);

    /**
     * Removes all children of this entry and returns pointer to the first one
     * Use child->next to access all others
     * @return
     */
    std::unique_ptr<FileEntry> pop_children();

    /**
     * Tries to find entry in tree starting from this entry (including it) by given FilePath
     * @param path - path to entry that should be found
     * @return pointer to entry if it was found, nullptr otherwise
     */
    FileEntry* findEntry(const FilePath* path);

    std::unique_ptr<FileEntry> pop_next()
    {
        return std::move(nextEntry);
    }

    int64_t deleteChildren();
    
    bool is_dir() const {
        return isDir;
    }

    bool isRoot() const {
        return parent == nullptr;
    }

private:

    void on_child_size_changed(FileEntry* child, int64_t sizeChange);

    /**
     * Pool for creating and caching entries.
     * Entries should not be destroyed
     */
    static std::unique_ptr<FileEntryPool> entryPool;

    FileEntry* parent;
    std::unique_ptr<FileEntry> firstChild;
    std::unique_ptr<FileEntry> nextEntry;
    bool isDir;
    uint16_t nameCrc;
    int64_t size;
    //not using std::string to reduce memory consumption (there are might be millions of entries so each byte counts)
    std::unique_ptr<char[]> name;


    friend class FileEntryPool;
};


#endif //SPACEDISPLAY_FILEENTRY_H
