
#ifndef SPACEDISPLAY_FILEENTRY_H
#define SPACEDISPLAY_FILEENTRY_H


#include <string>
#include <vector>

class FileEntryShared;
class FileEntryPool;

class FileEntry {

public:
    enum EntryType{
        DIRECTORY,
        FILE
    };
    
    FileEntry(uint64_t id, char *name, EntryType entryType);
    ~FileEntry();

    void reconstruct(uint64_t id, char* name, EntryType entryType);
    
    void set_size(int64_t size);
//    void set_path(char *path);
    
    EntryType get_type() const
    {
        return entryType;
    }

    const char* get_name() {
        return name;
    }
    
    void get_path(std::string& _path);
    int64_t get_size() {
        return size;
    }
    
    void set_parent(FileEntry* parent);
    
    FileEntry* get_parent() {
        return parent;
    }
    
    void add_child(FileEntry* child);
    
    size_t get_child_count() const
    {
        return childCount;
    }

    FileEntry* get_first_child()
    {
        return firstChild;
    }

    FileEntry* find_child_dir(const char *name);
    
    FileEntry* get_next()
    {
        return nextEntry;
    }

    void clear_entry(FileEntryPool* pool);

    //const std::vector<FileEntry*>&  get_children();
    
    void remove_child(FileEntry* child);
    
    bool is_dir() const {
        return isDir;
    }

private:
    
    void on_child_size_changed(FileEntry* child, int64_t sizeChange);
    
    EntryType  entryType;
    FileEntry* parent;
    FileEntry* firstChild;
    FileEntry* nextEntry;
    size_t childCount=0;
    bool isDir;
    uint64_t id;
    int64_t size;
    char* name;
//    char* path;


    friend class FileEntryPool;
    friend class FileEntryShared;
};


#endif //SPACEDISPLAY_FILEENTRY_H
