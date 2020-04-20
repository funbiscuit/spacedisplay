
#include <iostream>
#include <cstring>

#include "fileentry.h"
#include "filepath.h"
#include "fileentrypool.h"
#include "platformutils.h"
#include "utils.h"

extern "C" {
#include <crc.h>
}

std::unique_ptr<FileEntryPool> FileEntry::entryPool;

FileEntry::FileEntry(std::unique_ptr<char[]> name_, bool isDir_) :
        isDir(false), size(0), parent(nullptr), nameCrc(0)
{
    reconstruct(std::move(name_), isDir_);
}

void FileEntry::reconstruct(std::unique_ptr<char[]> name_, bool isDir_)
{
    size = 0;
    name = std::move(name_);
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
    //insert child in proper position to keep children sorted by size
    FileEntry* ch=firstChild.get();
    FileEntry* prev= nullptr;

    while (ch && ch->size>child->size)
    {
        prev=ch;
        ch=ch->nextEntry.get();
    }

    auto childSize = child->size;

    child->parent = this;
    if(prev)
    {
        child->nextEntry=std::move(prev->nextEntry);
        prev->nextEntry=std::move(child);
    }
    else
    {
        child->nextEntry = std::move(firstChild);
        firstChild=std::move(child);
    }
    size+=childSize;

    if(parent)
        parent->on_child_size_changed(this, childSize);
}

int64_t FileEntry::deleteChildren()
{
    int64_t childrenSize =0;
    auto ch=firstChild.get();
    while (ch!= nullptr)
    {
        childrenSize+=ch->size;
        ch=ch->nextEntry.get();
    }

    if(parent)
        parent->on_child_size_changed(this, -childrenSize);

    size -= childrenSize;

    return FileEntry::deleteEntryChain(std::move(firstChild));
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

void FileEntry::on_child_size_changed(FileEntry* child, int64_t sizeChange) {
    size+=sizeChange;
    
    auto ch=firstChild.get();
    FileEntry* prev= nullptr;
    FileEntry* oldPrev= nullptr;
    FileEntry* oldNext= nullptr;
    FileEntry* newPrev= nullptr;
    
    bool oldFound=false;
    bool newFound=false;
    
    while (ch!= nullptr)
    {
        if(ch==child)
        {
            oldPrev=prev;
            oldNext=ch->nextEntry.get();
            oldFound=true;
    
            if(newFound)
                break;
        }
        else if(!newFound && ch->size<child->size)
        {
            if(prev==child)
            {
                oldFound=false;
                break;
            }
            
            newFound=true;
            newPrev=prev;
            if(oldFound)
                break;
        }
        
        prev=ch;
        ch=ch->nextEntry.get();
    }
    
    //if this is a case - we are already at the end so don't need to move it
    if(!newFound && prev==child)
        oldFound=false;
    
    //usually we should always find old position, but sometimes we want to skip
    //this section by forcing oldFound to be false
    if(oldFound)
    {
        std::unique_ptr<FileEntry> childPtr;

        if(oldPrev!= nullptr)
        {
            //convert sequence oldPrev -> child -> oldNext to oldPrev -> oldNext
            childPtr = std::move(oldPrev->nextEntry);
            oldPrev->nextEntry=std::move(child->nextEntry);
        }
        else if(oldNext != nullptr)
        {
            // if old prev is null it means that child was the first one
            childPtr = std::move(firstChild);
            firstChild=std::move(child->nextEntry);
        }
    
        if(newPrev != nullptr)
        {
            //convert sequence newPrev -> newNext to newPrev -> child -> newNext
            child->nextEntry=std::move(newPrev->nextEntry);
            newPrev->nextEntry=std::move(childPtr);
        }
        else if(newFound)
        {
            //this is the biggest child
            child->nextEntry=std::move(firstChild);
            firstChild=std::move(childPtr);
        }
        else
        {
            //this is the smallest child
            child->nextEntry = nullptr;
            prev->nextEntry=std::move(childPtr);
        }
    }
    
    if(parent)
        parent->on_child_size_changed(this, sizeChange);
}

std::unique_ptr<FileEntry> FileEntry::pop_children()
{
    return std::move(firstChild);
}

FileEntry* FileEntry::findEntry(const FilePath* path)
{
    if(!path)
        return nullptr;

    auto& parts = path->getParts();
    if(parts.empty())
        return nullptr;
    auto& partCrcs = path->getCrs();

    //provided path should have the same root as name of this child since this function
    //should be called only from root entry
    if(parts.front() != name.get())
        return nullptr;

    auto currentParent = this;
    bool entryFound = true;
    for(int i=1;i<parts.size();++i)
    {
        auto child = currentParent->firstChild.get();
        auto& part = parts[i];
        bool isPartDir = part.back() == PlatformUtils::filePathSeparator;
        // part that is dir will have slash at the end so its length will be bigger by 1
        auto partLen = isPartDir ? (part.length()-1) : part.length();
        auto partCrc = partCrcs[i];

        entryFound = false;
        while(child)
        {
            //when crcs match in about 99.9% cases names will match either
            if(partCrc == child->nameCrc
               && strncmp(part.c_str(), child->name.get(), partLen) == 0
               && child->name[partLen] == '\0')
            {
                entryFound = true;
                currentParent = child;
                break;
            }

            child = child->nextEntry.get();
        }
    }

    return entryFound ? currentParent : nullptr;
}
