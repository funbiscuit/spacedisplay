
#ifndef SPACEDISPLAY_FILEENTRY_H
#define SPACEDISPLAY_FILEENTRY_H


#include <string>
#include <vector>
#include <memory>

class FileEntryView;
class FileEntryPool;
class FilePath;

class FileEntry {

public:
    FileEntry(std::unique_ptr<char[]> name_, bool isDir_);
    ~FileEntry();

    void reconstruct(std::unique_ptr<char[]> name_, bool isDir_);
    
    void set_size(int64_t size);

    const char* get_name() {
        return name.get();
    }
    
    void get_path(std::string& _path);
    void getPath(FilePath& _path);
    int64_t get_size() {
        return size;
    }

    uint16_t getNameCrc16() {
        return nameCrc;
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

    int64_t clear_entry(FileEntryPool* pool);
    
    bool is_dir() const {
        return isDir;
    }

private:
    
    void on_child_size_changed(FileEntry* child, int64_t sizeChange);
    
    FileEntry* parent;
    std::unique_ptr<FileEntry> firstChild;
    std::unique_ptr<FileEntry> nextEntry;
    bool isDir;
    uint16_t nameCrc; //actually 8bits would be enough, but it doesn't matter for object size
    int64_t size;
    //not using std::string to reduce memory consumption (there are might be millions of entries so each byte counts)
    std::unique_ptr<char[]> name;


    friend class FileEntryPool;
    friend class FileEntryView;
};


#endif //SPACEDISPLAY_FILEENTRY_H
