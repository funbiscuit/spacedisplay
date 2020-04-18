
#include <iostream>
#include <cstring>

#include "fileentry.h"
#include "filepath.h"
#include "fileentrypool.h"
#include "platformutils.h"

FileEntry::FileEntry(std::unique_ptr<char[]> name_, bool isDir_) :
        isDir(false), size(0), parent(nullptr)
{
    reconstruct(std::move(name_), isDir_);
}

void FileEntry::reconstruct(std::unique_ptr<char[]> name_, bool isDir_)
{
    size = 0;
    name = std::move(name_);
    isDir = isDir_;
    parent = nullptr;
}

FileEntry::~FileEntry() {
    //if name is valid, it's okay for it to destroy here but entry should be detached from main tree
    //but even in such case there should be no leak because both firstChild and nextEntry are
    //unique_ptr's so they will start destroying recursively
    if(firstChild)
        std::cerr << "Possible memory leak. Entry has children and was destroyed!\n";
    if(nextEntry)
        std::cerr << "Possible memory leak. Entry has next entry and was destroyed!\n";
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

int64_t FileEntry::clear_entry(FileEntryPool* pool)
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

    int64_t cachedEntries = 0;

    if(firstChild)
        cachedEntries = pool->cache_children(std::move(firstChild));

    return cachedEntries;
}

void FileEntry::get_path(std::string& _path) {

    if(parent)
    {
        parent->get_path(_path);

        if(_path.back()=='/' || _path.back()=='\\')
        {
            _path.append(name.get());
        }
        else
        {
            _path.append("/");
            _path.append(name.get());
        }
    }
    else
        _path = name.get();
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
        entryFound = false;
        while(child)
        {
            auto len = strlen(child->name.get());

            // part that is dir will have slash at the end so its length must be bigger by 1
            if(((isPartDir && part.length() == (len+1)) || (!isPartDir && part.length() == len))
               && strncmp(part.c_str(), child->name.get(), len) == 0)
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
