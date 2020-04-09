
#ifndef SPACEDISPLAY_FILEENTRYSHARED_H
#define SPACEDISPLAY_FILEENTRYSHARED_H

#include <QPainter>
#include <QPixmap>
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
    struct CopyOptions {
        int nestLevel = 3;
        int64_t minSize = 0;
        int64_t unknownSpace = 0; //make positive to include
        int64_t freeSpace = 0;    //make positive to include
    };
    enum class EntryType{
        DIRECTORY,
        FILE,
        AVAILABLE_SPACE,
        UNKNOWN_SPACE
    };

    ~FileEntryShared();

    static FileEntrySharedPtr create_copy(const FileEntry& entry, const CopyOptions& options);
    static void update_copy(FileEntrySharedPtr& copy, const FileEntry& entry, const CopyOptions& options);

    const char* get_name();

    const char* get_path(bool countRoot=true);
    int64_t get_size() {
        return size;
    }

    FileEntryShared* get_parent() {
        return parent;
    }

    EntryType get_type()
    {
        return entryType;
    }

    Utils::RectI get_draw_area()
    {
        return drawArea;
    }

    FileEntryShared* update_hovered_element(float mouseX, float mouseY);
    std::string format_size() const;
    std::string get_tooltip() const;
    std::string get_title();

    const std::vector<FileEntrySharedPtr>&  get_children();

    bool is_dir() const{
        return entryType==EntryType::DIRECTORY;
    }
    bool is_file() const{
        return entryType==EntryType::FILE;
    }

    int64_t get_id() const
    {
        return id;
    }

    QPixmap getNamePixmap(QPainter &painter, const QColor& color);
    QPixmap getSizePixmap(QPainter &painter, const QColor& color);
    QPixmap getTitlePixmap(QPainter &painter, const QColor& color);

    bool isHovered;
    bool isParentHovered{};
    void allocate_children(Utils::RectI rect, int titleHeight);
    void unhover();

private:
    /**
     * Makes a copy of existing object as shared pointer
     * Copy will not be erased when original entry is removed (so it is kept even after file entry pool reset)
     * @param entry original entry that is copied
     * @param nestLevel amount of nesting to copy (0 - only entry copied, 1 - entry+children, etc)
     * @param minSize minimum size of entry that should be copied. All other entries will not be copied
     */
    FileEntryShared(const FileEntry& entry, const CopyOptions& options);
    explicit FileEntryShared();

    void reconstruct_from(const FileEntry& entry, const CopyOptions& options);
    void init_from(const FileEntry& entry);

    void allocate_children2(size_t start, size_t end, std::vector<FileEntrySharedPtr> &bin, Utils::RectI &rect);
    void set_child_rect(const FileEntrySharedPtr& child, Utils::RectI &rect);

    void set_parent_hovered(bool hovered);
    void set_hovered(bool hovered);

    void createPixmapCache(QPainter &painter, const QColor& color);

    QPixmap cachedNamePix;
    std::string cachedName;
    QColor cachedNameColor;
    QPixmap cachedSizePix;
    std::string cachedSize;
    QColor cachedSizeColor;
    QPixmap cachedTitlePix;

    Utils::RectI drawArea;

    FileEntryShared* parent{};
    std::vector<FileEntrySharedPtr> children;
    uint64_t id = 0;
    int64_t size = 0;
    std::string name;
    EntryType  entryType = EntryType::FILE;
//    char* path;

};

#endif //SPACEDISPLAY_FILEENTRYSHARED_H
