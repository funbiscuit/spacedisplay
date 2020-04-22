
#include <iostream>
#include <cstring>

#include "fileentry.h"
#include "filepath.h"
#include "fileentrypool.h"
#include "utils.h"

extern "C" {
#include <crc.h>
}

std::unique_ptr<FileEntryPool> FileEntry::entryPool;

FileEntry::FileEntry(std::unique_ptr<char[]> name_, bool isDir_) :
        isDir(false), size(0), parent(nullptr), nameCrc(0), pathCrc(0)
{
    reconstruct(std::move(name_), isDir_);
}

void FileEntry::reconstruct(std::unique_ptr<char[]> name_, bool isDir_)
{
    size = 0;
    name = std::move(name_);
    pathCrc = nameCrc;
    nameCrc = crc16(name.get(), (uint16_t) strlen(name.get()));
    isDir = isDir_;
    parent = nullptr;
}

std::unique_ptr<FileEntry> FileEntry::createEntry(const std::string& name_, bool isDir_)
{
    // it should be safe to initialize here since entries should be created only from this function
    // so no entry methods could be called before this initialization
    //TODO probably should make thread safe
    if(!entryPool)
        entryPool = Utils::make_unique<FileEntryPool>();
    return entryPool->create_entry(name_, isDir_);
}

int64_t FileEntry::deleteEntryChain(std::unique_ptr<FileEntry> firstEntry)
{
    if(firstEntry)
        return entryPool->cache_children(std::move(firstEntry));
    return 0;
}

void FileEntry::set_size(int64_t size_) {
    size=size_;
}

void FileEntry::add_child(std::unique_ptr<FileEntry> child) {
    if(!child)
        return;

    auto childSize = child->size;
    //this will only insert child without changing our size or any callbacks to parent
    _addChild(std::move(child), true);
    size += childSize;

    if(parent)
        parent->on_child_size_changed(this, childSize);
}

void FileEntry::_addChild(std::unique_ptr<FileEntry> child, bool addCrc) {
    if(!child)
        return;

    const auto childSize = child->size;
    child->parent = this;
    // first try to find existing bin for appropriate size
    auto it = children.find(EntryBin(childSize));
    if(it!=children.end())
    {
        // if it exists, just add child to the front
        // cost cast is required because iterator returns const qualified bin
        // pointers don't affect sorting of children so this is safe
        auto& ptrFirst = const_cast<std::unique_ptr<FileEntry>&>(it->firstEntry);
        child->nextEntry = std::move(ptrFirst);
        ptrFirst = std::move(child);
    } else
    {
        // there is no bin with such size, so create new one
        children.insert(EntryBin(childSize, std::move(child)));
    }
}

int64_t FileEntry::deleteChildren()
{
    int64_t childrenSize = size;

    int64_t count = 0;
    for(auto& bin : children)
    {
        //reclaim pointer to entry chain
        auto p = std::unique_ptr<FileEntry>(
                const_cast<std::unique_ptr<FileEntry>&>(bin.firstEntry).release());

        count += deleteEntryChain(std::move(p));
    }
    children.clear();
    size -= childrenSize;

    if(parent)
        parent->on_child_size_changed(this, -childrenSize);

    return count;
}

void FileEntry::getPath(FilePath& _path) {

    if(parent)
    {
        parent->getPath(_path);
        if(isDir)
            _path.addDir(name.get(), nameCrc);
        else
            _path.addFile(name.get(), nameCrc);
    }
    else
        _path.setRoot(name.get(), nameCrc);
}

int64_t FileEntry::get_size() const
{
    return size;
}

uint16_t FileEntry::getNameCrc16() const
{
    return nameCrc;
}

void FileEntry::updatePathCrc(uint16_t parentPathCrc)
{
    pathCrc = parentPathCrc ^ nameCrc;
}

const char* FileEntry::getName() const
{
    return name.get();
}

bool FileEntry::forEach(const std::function<bool(const FileEntry&)>& func) const
{
    if(children.empty())
        return false;

    for(auto& bin : children)
    {
        bool stop = false;
        auto child = bin.firstEntry.get();
        while(child != nullptr && !stop)
        {
            stop = !func(*child);
            child = child->nextEntry.get();
        }

        if(stop)
            break;
    }

    return true;
}

void FileEntry::on_child_size_changed(FileEntry* child, int64_t sizeChange) {

    if(!child || sizeChange == 0)
        return;

    //size of child changed so we should use its previous size
    auto it = children.find(EntryBin(child->size-sizeChange));

    if(it == children.end())
    {
        //should not happen
        std::cerr << "Can't find child in parents children!\n";
        return;
    }

    auto first = it->firstEntry.get();

    std::unique_ptr<FileEntry> childPtr;

    if(first == child)
    {
        auto& ptrFirst = const_cast<std::unique_ptr<FileEntry>&>(it->firstEntry);
        childPtr = std::move(ptrFirst);
        if(childPtr->nextEntry)
            ptrFirst = std::move(childPtr->nextEntry);
        else
        {
            //this was the only child in bin so we should delete bin
            children.erase(it);
        }
    } else
    {
        //find child in entry chain
        while(first != nullptr && first->nextEntry.get() != child)
            first = first->nextEntry.get();

        if(first != nullptr)
        {
            childPtr = std::move(first->nextEntry);
            first->nextEntry = std::move(childPtr->nextEntry);
        }
    }

    if(!childPtr)
    {
        //should not happen
        std::cerr << "Can't find child in parents children!\n";
        return;
    }

    // now just add child back
    _addChild(std::move(childPtr), false);
    size += sizeChange;

    if(parent)
        parent->on_child_size_changed(this, sizeChange);
}
