
#ifndef SPACEDISPLAY_FILEENTRY_H
#define SPACEDISPLAY_FILEENTRY_H


#include <string>
#include <vector>
#include <memory>

class FileEntryShared;
class FileEntryPool;

class FileEntry {

public:
    enum EntryType{
        DIRECTORY,
        FILE
    };
    
    FileEntry(uint64_t id, std::unique_ptr<char[]> name, EntryType entryType);
    ~FileEntry();

    void reconstruct(uint64_t id_, std::unique_ptr<char[]> name_, EntryType entryType_);
    
    void set_size(int64_t size);

    const char* get_name() {
        return name.get();
    }
    
    void get_path(std::string& _path);
    int64_t get_size() {
        return size;
    }

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

    FileEntry* find_child_dir(const char *name_);

    std::unique_ptr<FileEntry> pop_next()
    {
        return std::move(nextEntry);
    }

    void clear_entry(FileEntryPool* pool);
    
    bool is_dir() const {
        return isDir;
    }

private:
    
    void on_child_size_changed(FileEntry* child, int64_t sizeChange);
    
    EntryType  entryType;
    FileEntry* parent;
    std::unique_ptr<FileEntry> firstChild;
    std::unique_ptr<FileEntry> nextEntry;
    size_t childCount=0;
    bool isDir;
    uint64_t id;
    int64_t size;
    //not using std::string to reduce memory consumption (there are might be millions of entries so each byte counts)
    std::unique_ptr<char[]> name;


    friend class FileEntryPool;
    friend class FileEntryShared;
};


#endif //SPACEDISPLAY_FILEENTRY_H
