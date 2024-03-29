
#include <iostream>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <cmath>

#include "filepath.h"
#include "fileentry.h"
#include "fileentryview.h"
#include "platformutils.h"

const int MAX_CHILD_COUNT = 100;
const int MIN_CHILD_PIXEL_AREA = 50;

// this counter is enough for generating 10 million IDs for 58 years
uint64_t FileEntryView::idCounter = 0;

FileEntryView::FileEntryView() : drawArea{} {
    id = ++idCounter;
}


FileEntryView::FileEntryView(const FileEntry *entry, const ViewOptions &options) : FileEntryView() {
    reconstruct_from(entry, options);
}


FileEntryView::~FileEntryView() {

}

void FileEntryView::init_from(const FileEntry *entry) {
    size = entry->getSize();
    name = entry->getName();
    id = ++idCounter;
    entryType = entry->isDir() ? EntryType::DIRECTORY : EntryType::FILE;
    drawArea.x = 0;
    drawArea.y = 0;
    drawArea.w = 0;
    drawArea.h = 0;
}

void FileEntryView::reconstruct_from(const FileEntry *entry, const ViewOptions &options) {
    init_from(entry);
    parent = nullptr;

    auto unknownSpace = options.unknownSpace;
    auto freeSpace = options.freeSpace;

    if (options.nestLevel > 0) {
        size_t childCount = 0;
        size_t existingChildCount = children.size();
        entry->forEach([this, &options, &childCount, &existingChildCount, &unknownSpace, &freeSpace](
                const FileEntry &child) -> bool {

            if (childCount >= MAX_CHILD_COUNT)
                return false;

            auto childSize = child.getSize();
            if (childSize < options.minSize)
                return false;

            //TODO if unknown space is less than free space, but they are both bigger than biggest child
            // it will still be included first (this is not critical)
            if (unknownSpace > childSize) {
                FileEntryViewPtr newChild;
                if (childCount < existingChildCount) {
                    newChild = children[childCount];
                } else {
                    newChild = FileEntryViewPtr(new FileEntryView());
                    children.push_back(newChild);
                }
                newChild->parent = this;
                newChild->entryType = EntryType::UNKNOWN_SPACE;
                newChild->size = unknownSpace;
                newChild->name.clear();
                newChild->children.clear();
                ++childCount;
                unknownSpace = 0;
            }
            if (freeSpace > childSize) {
                FileEntryViewPtr newChild;
                if (childCount < existingChildCount) {
                    newChild = children[childCount];
                } else {
                    newChild = FileEntryViewPtr(new FileEntryView());
                    children.push_back(newChild);
                }
                newChild->parent = this;
                newChild->entryType = EntryType::AVAILABLE_SPACE;
                newChild->size = freeSpace;
                newChild->name.clear();
                newChild->children.clear();
                ++childCount;
                freeSpace = 0;
            }


            ViewOptions newOptions = options;
            --newOptions.nestLevel;
            //we don't need to include unknown and free space to child entries
            newOptions.freeSpace = 0;
            newOptions.unknownSpace = 0;

            if (childCount < existingChildCount) {
                updateView(children[childCount], &child, newOptions);
                children[childCount]->parent = this;
            } else {
                auto newChild = createView(&child, newOptions);
                newChild->parent = this;
                children.push_back(newChild);
            }
            ++childCount;
            return true;
        });

        while (existingChildCount > childCount) {
            children.pop_back();
            --existingChildCount;
        }

    } else {
        children.clear();
    }
}

std::string FileEntryView::get_tooltip() const {
    std::string sizeStr = getFormattedSize();
    std::string str;

    if (is_dir())
        str = Utils::strFormat("<center>%s<br />%s<br />(Click to zoom)</center>", name.c_str(), sizeStr.c_str());
    else if (is_file())
        str = Utils::strFormat("<center>%s<br />%s</center>", name.c_str(), sizeStr.c_str());

    return str;
}

FileEntryView *FileEntryView::getHoveredView(int mouseX, int mouseY) {
    if (drawArea.x <= mouseX && drawArea.y <= mouseY &&
        drawArea.w >= mouseX - drawArea.x && drawArea.h >= mouseY - drawArea.y) {
        for (const auto &child :children) {
            auto hoveredChild = child->getHoveredView(mouseX, mouseY);
            if (hoveredChild)
                return hoveredChild;
        }
        return this;
    }
    return nullptr;
}

FileEntryView *FileEntryView::getClosestView(const FilePath &filepath, int maxDepth) {
    auto closest = this;
    int currentDepth = 0;

    auto &parts = filepath.getParts();

    while (currentDepth < maxDepth && currentDepth < parts.size()) {
        bool found = false;
        auto part = parts[currentDepth];
        if (part.back() == PlatformUtils::filePathSeparator)
            part.pop_back();

        for (auto &child : closest->children) {
            //find appropriate child with big enough area
            if (child->drawArea.w > MIN_CHILD_PIXEL_AREA / 5
                && child->drawArea.h > MIN_CHILD_PIXEL_AREA / 5
                && child->name == part) {
                closest = child.get();
                found = true;
                break;
            }
        }

        if (!found)
            break;

        ++currentDepth;
    }

    return closest;
}

std::string FileEntryView::getFormattedSize() const {
    return Utils::formatSize(size);
}

void FileEntryView::allocate_view(Utils::RectI rect, int titleHeight) {
    if (children.empty() || rect.h < titleHeight || rect.w < 1)
        return;

    drawArea = rect;

    rect.y += titleHeight + 2;
    rect.h -= titleHeight + 2 + 2;
    rect.x += 2;
    rect.w -= 4;


    allocate_children(0, children.size() - 1, rect);
    for (const auto &child:children) {
        child->allocate_view(child->drawArea, titleHeight);
    }
}

const char *FileEntryView::getName() const {
    switch (entryType) {
        case EntryType::AVAILABLE_SPACE:
            return "Free";
        case EntryType::UNKNOWN_SPACE:
            return "Unknown";
        default:
            return name.c_str();
    }
}

void FileEntryView::allocate_children(size_t start, size_t end, Utils::RectI &rect) {
    if (rect.h < 1 || rect.w < 1)
        return;
    if (end - start < 0)
        return;
    else if ((end - start) == 0) {
        set_child_rect(children[start], rect);
        return;
    }

    int64_t totalSize = 0;
    for (size_t i = start; i <= end; ++i) {
        totalSize += children[i]->size;
    }

    int availableArea = rect.h * rect.w;
    auto minPart = MIN_CHILD_PIXEL_AREA / availableArea;
    auto minSize = int64_t(totalSize * minPart);
    size_t newEnd = start;
    for (size_t i = start; i <= end; ++i) {
        if (children[i]->size < minSize && i > start + 1)
            break;
        ++newEnd;
    }
    for (size_t i = newEnd; i <= end; ++i) {
        children[i]->drawArea.w = 0.f;
        children[i]->drawArea.h = 0.f;
    }
    if (newEnd == start)
        return;
    end = newEnd - 1;

    size_t bin1last = start;
    int64_t sum1 = children[start]->size;
    int64_t sum2 = 0;

    for (size_t i = start + 1; i <= end; ++i)
        sum2 += children[i]->size;

    bool bin1Filled = false;

    for (size_t i = start + 1; i <= end; ++i) {
        auto curSize = children[i]->size;
        if (!bin1Filled && std::abs(sum2 - sum1 - curSize * 2) < std::abs(sum2 - sum1)) {
            ++bin1last;
            sum1 += curSize;
            sum2 -= curSize;
        } else {
            bin1Filled = true;
        }
    }

    if (sum1 == 0 && sum2 == 0) {
        sum1 = 1;
        sum2 = 1;
    }

    //float a1=float(sum1)/float(sum1+sum2);


    bool divX = rect.w >= rect.h;

    int rX = rect.x;
    int rY = rect.y;
    int64_t rW = rect.w;
    int64_t rH = rect.h;
    int midW1 = static_cast<int>((rW * sum1) / (sum1 + sum2));
    int midH1 = static_cast<int>((rH * sum1) / (sum1 + sum2));

    if (divX) {
        Utils::RectI rect2{};
        rect2.x = rX;
        rect2.y = rY;
        rect2.w = midW1;
        rect2.h = rH;

        if ((bin1last - start) > 0)
            allocate_children(start, bin1last, rect2);
        else
            set_child_rect(children[start], rect2);

        rect2.x = rX + midW1;
        rect2.w = static_cast<int>(rW - midW1);
        if ((end - bin1last) > 1)
            allocate_children(bin1last + 1, end, rect2);
        else
            set_child_rect(children[end], rect2);
    } else {
        Utils::RectI rect2{};
        rect2.x = rX;
        rect2.y = rY;
        rect2.w = rW;
        rect2.h = midH1;

        if ((bin1last - start) > 0)
            allocate_children(start, bin1last, rect2);
        else
            set_child_rect(children[start], rect2);

        rect2.y = rY + midH1;
        rect2.h = static_cast<int>(rH - midH1);
        if ((end - bin1last) > 1)
            allocate_children(bin1last + 1, end, rect2);
        else
            set_child_rect(children[end], rect2);
    }
}

void FileEntryView::set_child_rect(const FileEntryViewPtr &child, Utils::RectI &rect) {
    child->drawArea = rect;

    if (child->drawArea.w < 1 || child->drawArea.h < 1) {
        child->drawArea.w = 0;
        child->drawArea.h = 0;
    }
}

FileEntryViewPtr FileEntryView::createView(const FileEntry *entry, const ViewOptions &options) {
    return FileEntryViewPtr(new FileEntryView(entry, options));
}

void FileEntryView::updateView(FileEntryViewPtr &copy, const FileEntry *entry, const ViewOptions &options) {
    copy->reconstruct_from(entry, options);
}

void FileEntryView::getPath(FilePath &root) const {
    if (parent) {
        parent->getPath(root);
        if (is_dir())
            root.addDir(name);
        else
            root.addFile(name);
        return;
    }
    // if this entry doesn't have parent then root should be the path pointing to this entry
//    std::cout << "Two lines should be the same:\n";
//    std::cout << root.getName() <<"\n";
//    std::cout << name << "\n";
}

const std::vector<FileEntryViewPtr> &FileEntryView::get_children() const {
    return children;
}

uint64_t FileEntryView::getId() const {
    return id;
}
