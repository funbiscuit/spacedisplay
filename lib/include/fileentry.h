
#ifndef SPACEDISPLAY_FILEENTRY_H
#define SPACEDISPLAY_FILEENTRY_H


#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <set>
#include <unordered_map>

class FileEntry {
public:
    /**
     * Constructs new FileEntry object.
     * Name must be non empty
     * @param name
     * @param isDir
     * @param size
     * @throws std::invalid_argument if name is empty
     */
    FileEntry(const std::string &name, bool isDir, int64_t size = 0);

    /**
     * Sets new size for this entry.
     * If entry has a parent, this will be a slow operation
     * since parent will need to reorder this entry
     * @param newSize
     */
    void setSize(int64_t newSize);

    int64_t getSize() const;

    const char *getName() const;

    const FileEntry *getParent() const;

    uint16_t getNameCrc() const;

    uint16_t getPathCrc() const;

    /**
     * Executes provided function for each child until all children are processed
     * If function returns false, processing is stopped
     * @param func
     * @return false if entry doesn't have children, true otherwise
     */
    bool forEach(const std::function<bool(const FileEntry &)> &func) const;

    /**
     * Adds child to children of this entry.
     * Adds to relevant place so all children are sorted by size (in decreasing order)
     * @param child
     */
    void addChild(std::unique_ptr<FileEntry> child);

    /**
     * Remove all children, that are marked for deletion
     * All removed children are put into provided vector
     */
    void removePendingDelete(std::vector<std::unique_ptr<FileEntry>> &deletedChildren);

    /**
     * Mark all children for deletion.
     * All marked children will be deleted with call to removePendingDelete()
     * @param files - how much files are marked
     * @param dirs - how much directories are marked
     */
    void markChildrenPendingDelete(int &files, int &dirs);

    /**
     * Unmark this entry so it is not deleted with removePendingDelete()
     */
    void unmarkPendingDelete();

    bool isDir() const;

    bool isRoot() const;

private:

    void _addChild(std::unique_ptr<FileEntry> child);

    void onChildSizeChanged(FileEntry *child, int64_t sizeChange);

    /**
     * Recalculates crc of path to this entry given path crc of entry parent
     * @param parentPathCrc
     */
    void updatePathCrc(uint16_t parentPathCrc);

    struct EntryBin {
        int64_t size;
        // entries are in chain (all entries in chain have the same size)
        // to access next one use firstEntry->next
        std::unique_ptr<FileEntry> firstEntry;

        explicit EntryBin(int64_t size, std::unique_ptr<FileEntry> p = nullptr) :
                size(size), firstEntry(std::move(p)) {}

        bool operator<(const EntryBin &right) const {
            // use ordering in decreasing order
            return size > right.size;
        }
    };

    FileEntry *parent;

    //used only inside EntryBin to point to the next entry with the same size
    std::unique_ptr<FileEntry> nextEntry;
    std::set<EntryBin> children;

    //used to mark entry to delete in function removePendingDelete
    bool pendingDelete;

    bool bIsDir;
    uint16_t nameCrc;

    // path crc is xor of all names in path (without trailing slashes, except root)
    uint16_t pathCrc;
    int64_t size;
    //not using std::string to reduce memory consumption (there are might be millions of entries so each byte counts)
    std::unique_ptr<char[]> name;
};


#endif //SPACEDISPLAY_FILEENTRY_H
