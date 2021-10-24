
#ifndef SPACEDISPLAY_FILEENTRYVIEW_H
#define SPACEDISPLAY_FILEENTRYVIEW_H

#include <QPainter>
#include <QPixmap>
#include <memory>
#include <vector>
#include <string>
#include "utils.h"

class FileEntryView;

class FilePath;

class FileEntry;

/**
 * This class is intended to be used only inside shared pointer object!
 */
typedef std::shared_ptr<FileEntryView> FileEntryViewPtr; // nice short alias
class FileEntryView {

public:
    enum CreateCopyFlags : uint16_t {
        DEFAULT = 0x00,
        INCLUDE_AVAILABLE_SPACE = 0x01,
        INCLUDE_UNKNOWN_SPACE = 0x02
    };
    struct ViewOptions {
        int nestLevel = 3;
        int64_t minSize = 0;
        int64_t unknownSpace = 0; //make positive to include
        int64_t freeSpace = 0;    //make positive to include
    };
    enum class EntryType {
        DIRECTORY,
        FILE,
        AVAILABLE_SPACE,
        UNKNOWN_SPACE
    };

    FileEntryView(const FileEntryView &) = delete;

    ~FileEntryView();

    static FileEntryViewPtr createView(const FileEntry *entry, const ViewOptions &options);

    static void updateView(FileEntryViewPtr &copy, const FileEntry *entry, const ViewOptions &options);

    const char *getName() const;

    /**
     * Writes path to this entry to provided local root.
     * For example, if local hierarchy is {this entry}<-{dir1}<-{dir2}<-{dir3}
     * Then provided root should have path to {dir3}. And returned path will be {root}/dir2/dir1/{entry name}
     * @param root - FilePath to local root. 
     */
    void getPath(FilePath &root) const;

    int64_t get_size() const {
        return size;
    }

    FileEntryView *get_parent() const {
        return parent;
    }

    EntryType get_type() const {
        return entryType;
    }

    Utils::RectI get_draw_area() const {
        return drawArea;
    }

    FileEntryView *getHoveredView(int mouseX, int mouseY);

    /**
     * Returns closest view to specified path. For example,
     * if C:\Windows\System32 is specified, but only C:\Windows exist, then it
     * will be returned.
     * @param filepath
     * @param maxDepth
     * @return
     */
    FileEntryView *getClosestView(const FilePath &filepath, int maxDepth);

    std::string getFormattedSize() const;

    std::string get_tooltip() const;

    std::string getTitle(const char *path = nullptr) const;

    const std::vector<FileEntryViewPtr> &get_children() const;

    uint64_t getId() const;

    bool is_dir() const {
        return entryType == EntryType::DIRECTORY;
    }

    bool is_file() const {
        return entryType == EntryType::FILE;
    }

    /**
     * Allocates this view and all its children inside specified rectangle.
     * Reserves space for titlebar of specified height.
     * @param rect - rectangle inside of which view should be allocated
     * @param titleHeight - how much space to reserve for title
     */
    void allocate_view(Utils::RectI rect, int titleHeight);

private:
    /**
     * Makes a copy of existing object as shared pointer
     * Copy will not be erased when original entry is removed (so it is kept even after file entry pool reset)
     * @param entry original entry that is copied
     * @param nestLevel amount of nesting to copy (0 - only entry copied, 1 - entry+children, etc)
     * @param minSize minimum size of entry that should be copied. All other entries will not be copied
     */
    FileEntryView(const FileEntry *entry, const ViewOptions &options);

    explicit FileEntryView();

    void reconstruct_from(const FileEntry *entry, const ViewOptions &options);

    void init_from(const FileEntry *entry);

    /**
     * Allocates children of this view inside specified rectangle
     * Allocates only children in range from start to end (both inclusive)
     * @param start
     * @param end
     * @param rect
     */
    void allocate_children(size_t start, size_t end, Utils::RectI &rect);

    void set_child_rect(const FileEntryViewPtr &child, Utils::RectI &rect);

    Utils::RectI drawArea;

    FileEntryView *parent{};
    std::vector<FileEntryViewPtr> children;
    int64_t size = 0;
    uint64_t id = 0;
    std::string name;
    EntryType entryType = EntryType::FILE;

    static uint64_t idCounter;
};

#endif //SPACEDISPLAY_FILEENTRYVIEW_H
