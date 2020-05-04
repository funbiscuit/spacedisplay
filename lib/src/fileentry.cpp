
#include <iostream>
#include <cstring>

#include "fileentry.h"
#include "filepath.h"
#include "utils.h"

extern "C" {
#include <crc.h>
}

FileEntry::FileEntry(const std::string& name_, bool isDir_, int64_t size_) :
        bIsDir(isDir_), pendingDelete(false), parent(nullptr),
        nameCrc(0), pathCrc(0), size(size_)
{
    auto nameLen=name_.length();
    if(nameLen == 0)
        throw std::invalid_argument("Can't create FileEntry with empty name");

    auto chars=Utils::make_unique_arr<char>(nameLen+1);
    memcpy(chars.get(), name_.c_str(), (nameLen+1)*sizeof(char));
    nameCrc = crc16(chars.get(), (uint16_t) nameLen);
    pathCrc = nameCrc;
    name = std::move(chars);
}

void FileEntry::addChild(std::unique_ptr<FileEntry> child) {
    if(!child)
        return;

    auto childSize = child->size;
    //this will only insert child without changing our size or any callbacks to parent
    _addChild(std::move(child));
    size += childSize;

    if(parent)
        parent->onChildSizeChanged(this, childSize);
}

void FileEntry::removePendingDelete(std::vector<std::unique_ptr<FileEntry>>& deletedChildren)
{
    int64_t changedSize = 0;
    auto it = children.begin();
    while(it!=children.end())
    {
        if(!it->firstEntry) //this shouldn't happen, but better check
            it = children.erase(it);

        auto child = it->firstEntry.get();
        while(child->nextEntry)
        {
            if(child->nextEntry->pendingDelete)
            {
                auto temp = std::move(child->nextEntry);
                child->nextEntry = std::move(temp->nextEntry);
                changedSize += temp->size;
                deletedChildren.push_back(std::move(temp));
                continue;
            }
            child = child->nextEntry.get();
        }
        //now all children in this bin (except first) are processed
        //now check if first child should also be removed and if it is, check if we have anything left
        if(it->firstEntry->pendingDelete)
        {
            auto& ptrFirst = const_cast<std::unique_ptr<FileEntry>&>(it->firstEntry);
            auto temp = std::move(ptrFirst);
            if(temp->nextEntry)
                ptrFirst = std::move(temp->nextEntry);
            changedSize += temp->size;
            deletedChildren.push_back(std::move(temp));
        }
        //if no entries left, delete bin
        if(!it->firstEntry)
            it = children.erase(it);
        else
            ++it;
    }
    if(changedSize>0)
    {
        size-=changedSize;
        if(parent)
            parent->onChildSizeChanged(this, -changedSize);
    }
}

void FileEntry::_addChild(std::unique_ptr<FileEntry> child) {
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

int64_t FileEntry::getSize() const
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

void FileEntry::onChildSizeChanged(FileEntry* child, int64_t sizeChange) {

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
    _addChild(std::move(childPtr));
    size += sizeChange;

    if(parent)
        parent->onChildSizeChanged(this, sizeChange);
}
