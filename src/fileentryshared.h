
#ifndef SPACEDISPLAY_FILEENTRYSHARED_H
#define SPACEDISPLAY_FILEENTRYSHARED_H

#include <memory>
#include <vector>
#include <string>
#include "utils.h"
#include "fileentry.h"

class FileEntryShared;

/**
 * This class is intended to be used only inside shared pointer object!
 */
typedef std::shared_ptr<FileEntryShared> FileEntrySharedPtr; // nice short alias
class FileEntryShared {

public:
    enum CreateCopyFlags : uint16_t {
        DEFAULT =0x00,
        INCLUDE_AVAILABLE_SPACE = 0x01,
        INCLUDE_UNKNOWN_SPACE = 0x02
    };

    ~FileEntryShared();

    static FileEntrySharedPtr create_copy(const FileEntry& entry, int nestLevel, int64_t minSize, int flags);
    static void update_copy(FileEntrySharedPtr& copy, const FileEntry& entry, int nestLevel, int64_t minSize, int flags);

    const char* get_name();

    const char* get_path(bool countRoot=true);
    int64_t get_size() {
        return size;
    }

    FileEntryShared* get_parent() {
        return parent;
    }

    FileEntry::EntryType get_type()
    {
        return entryType;
    }

    Utils::Rect get_draw_area()
    {
        return drawArea;
    }

    FileEntryShared* update_hovered_element(float mouseX, float mouseY);
    std::string format_size() const;
    std::string get_tooltip() const;
    std::string get_title();

    const std::vector<FileEntrySharedPtr>&  get_children();

    bool is_dir() const{
        return entryType==FileEntry::DIRECTORY;
    }
    bool is_file() const{
        return entryType==FileEntry::FILE;
    }

    int64_t get_id() const
    {
        return id;
    }

    bool isHovered;
    bool isParentHovered;
    void allocate_children(Utils::Rect rect, float titleHeight);
    void unhover();

private:
    /**
     * Makes a copy of existing object as shared pointer
     * Copy will not be erased when original entry is removed (so it is kept even after file entry pool reset)
     * @param entry original entry that is copied
     * @param nestLevel amount of nesting to copy (0 - only entry copied, 1 - entry+children, etc)
     * @param minSize minimum size of entry that should be copied. All other entries will not be copied
     */
    FileEntryShared(const FileEntry& entry, int nestLevel, int64_t minSize, int flags);
    explicit FileEntryShared(FileEntry::EntryType type);

    void reconstruct_from(const FileEntry& entry, int nestLevel, int64_t minSize, uint16_t flags);
    void init_from(const FileEntry& entry);

    void allocate_children2(size_t start, size_t end, std::vector<FileEntrySharedPtr> &bin, Utils::Rect &rect);
    void set_child_rect(const FileEntrySharedPtr& child, Utils::Rect &rect);

    void set_parent_hovered(bool hovered);
    void set_hovered(bool hovered);


    Utils::Rect drawArea;

    FileEntryShared* parent;
    std::vector<FileEntrySharedPtr> children;
    uint64_t id;
    int64_t size;
    std::string name;
    FileEntry::EntryType  entryType;
//    char* path;

};

#endif //SPACEDISPLAY_FILEENTRYSHARED_H
