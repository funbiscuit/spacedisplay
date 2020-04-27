
#ifndef SPACEDISPLAY_FILEENTRY_H
#define SPACEDISPLAY_FILEENTRY_H


#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <set>
#include <unordered_map>

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

    uint16_t getNameCrc16() const;

    /**
     * Executes provided function for each child until all children are processed
     * If function returns false, processing is stopped
     * @param func
     * @return false if entry doesn't have children, true otherwise
     */
    bool forEach(const std::function<bool(const FileEntry&)>& func) const;

    /**
     * Recalculates crc of path to this entry given path crc of entry parent
     * @param parentPathCrc
     */
    void updatePathCrc(uint16_t parentPathCrc);

    /**
     * Adds child to children of this entry.
     * Adds to relevant place so all children are sorted by size (in decreasing order)
     * @param child
     */
    void add_child(std::unique_ptr<FileEntry> child);

    /**
     * Remove all children, that are marked for deletion
     * All removed children are put into provided vector
     */
    void removePendingDelete(std::vector<std::unique_ptr<FileEntry>>& deletedChildren);

    int64_t deleteChildren();
    
    bool is_dir() const {
        return isDir;
    }

    bool isRoot() const {
        return parent == nullptr;
    }

private:

    void _addChild(std::unique_ptr<FileEntry> child, bool addCrc);

    void on_child_size_changed(FileEntry* child, int64_t sizeChange);

    struct EntryBin {
        uint64_t size;
        // entries are in chain (all entries in chain have the same size)
        // to access next one use firstEntry->next
        std::unique_ptr<FileEntry> firstEntry;
        explicit EntryBin(uint64_t size, std::unique_ptr<FileEntry> p = nullptr) :
                size(size), firstEntry(std::move(p)) {}

        bool operator< (const EntryBin& right) const {
            // use ordering in decreasing order
            return size > right.size;
        }
    };

    /**
     * Pool for creating and caching entries.
     * Entries should not be destroyed
     */
    static std::unique_ptr<FileEntryPool> entryPool;

    FileEntry* parent;

    //used only inside EntryBin to point to the next entry with the same size
    std::unique_ptr<FileEntry> nextEntry;
    std::set<EntryBin> children;

    //used to mark entry to delete in function removePendingDelete
    bool pendingDelete;

    bool isDir;
    uint16_t nameCrc;

    // path crc is xor of all names in path (without trailing slashes, except root)
    uint16_t pathCrc;
    int64_t size;
    //not using std::string to reduce memory consumption (there are might be millions of entries so each byte counts)
    std::unique_ptr<char[]> name;


    friend class FileEntryPool;
    friend class FileDB;
};


#endif //SPACEDISPLAY_FILEENTRY_H
