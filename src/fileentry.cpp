
#include <iostream>
#include <cstring>

#include "fileentry.h"
#include "fileentrypool.h"

FileEntry::FileEntry(uint64_t id, char *name, EntryType entryType)
{
    reconstruct(id, name, entryType);
}

void FileEntry::reconstruct(uint64_t id, char* name, EntryType entryType)
{
    this->id = id;
    this->size = 0;
    this->name = name;
    this->isDir = entryType==DIRECTORY;
    this->parent = nullptr;
    this->firstChild = nullptr;
    this->nextEntry = nullptr;
    this->childCount = 0;
    this->entryType = entryType;
}

FileEntry::~FileEntry() {
    //name will be deleted by pool allocator
    if(name /*&& name!=path*/)
    {
        //delete(name);
        name=nullptr;
    }
//    if(path)
//    {
//        delete(path);
//        path=nullptr;
//    }
}

void FileEntry::set_size(int64_t size) {
    this->size=size;
}

void FileEntry::set_parent(FileEntry *parent) {
    if(this->parent)
        this->parent->remove_child(this);
    
    this->parent=parent;
    parent->add_child(this);
}

void FileEntry::add_child(FileEntry *child) {
    if(!child)
        return;
    //insert child in proper position to keep children sorted by size
    
    auto ch=firstChild;
    FileEntry* prev= nullptr;

    while (ch!= nullptr && ch->size>child->size)
    {
        prev=ch;
        ch=ch->nextEntry;
    }
    
    child->nextEntry=ch;
    if(prev)
        prev->nextEntry=child;
    else
        firstChild=child;
    childCount++;

    size+=child->size;
    if(parent)
        parent->on_child_size_changed(this, child->size);
}

void FileEntry::clear_entry(FileEntryPool* pool)
{
    int64_t childrenSize =0;
    auto ch=firstChild;
    while (ch!= nullptr)
    {
        childrenSize+=ch->size;
        ch=ch->nextEntry;
    }

    if(parent)
        parent->on_child_size_changed(this, -childrenSize);

    size -= childrenSize;

    if(firstChild)
        pool->cache_children(firstChild);

    firstChild = nullptr;
}

void FileEntry::remove_child(FileEntry *child) {

}

//void FileEntry::set_path(char *path) {
//    if(this->path)
//        delete(this->path);
//
//    this->path =path;
//}

void FileEntry::get_path(std::string& _path) {

    if(parent)
    {
        parent->get_path(_path);

        if(_path.back()=='/' || _path.back()=='\\')
        {
            _path.append(name);
        }
        else
        {
            _path.append("/");
            _path.append(name);
        }
    }
    else
        _path = name;
}

void FileEntry::on_child_size_changed(FileEntry* child, int64_t sizeChange) {
    size+=sizeChange;
    
    auto ch=firstChild;
    FileEntry* prev= nullptr;
    FileEntry* oldPrev= nullptr;
    FileEntry* oldNext= nullptr;
    FileEntry* newPrev= nullptr;
    FileEntry* newNext= nullptr;
    
    bool oldFound=false;
    bool newFound=false;
    
    while (ch!= nullptr)
    {
        if(ch==child)
        {
            oldPrev=prev;
            oldNext=ch->nextEntry;
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
            newNext=ch;
            if(oldFound)
                break;
        }
        
        prev=ch;
        ch=ch->nextEntry;
    }
    
    //if this is a case - we are already at the end so don't need to move it
    if(!newFound && prev==child)
        oldFound=false;
    
    //usually we should always found old position, but sometimes we want to skip
    //this section by forcing oldFound to be false
    if(oldFound)
    {
        if(oldPrev!= nullptr)
            oldPrev->nextEntry=oldNext;
        else if(oldNext != nullptr)
            firstChild=oldNext;
    
        if(newPrev!= nullptr)
        {
            child->nextEntry=newNext;
            newPrev->nextEntry=child;
        }
        else if(newFound)
        {
            //this is the biggest child
            child->nextEntry=newNext;
            firstChild=child;
        }
        else
        {
            //this is the smallest child
            child->nextEntry= nullptr;
            prev->nextEntry=child;
        }
    }
    
    if(parent)
        parent->on_child_size_changed(this, sizeChange);
}

FileEntry *FileEntry::find_child_dir(const char *name) {
    auto child=firstChild;

    size_t n=strlen(name);
    size_t tn=strlen(this->name);

    while(n>0 && name[0]=='/')
    {
        name = name + sizeof(char);
        --n;
    }

//    if(tn>n)
//        return nullptr;
//
//    if(strncmp(this->name, name, tn)==0)
//    {
//        name=name+tn*sizeof(char);
//        n -= tn;
//    }


    while (child != nullptr)
    {
        if(!child->is_dir())
        {
            child=child->nextEntry;
            continue;
        }

        size_t cn=strlen(child->name);
        if(strncmp(child->name, name, cn)==0 && (n==cn || name[cn]=='/'))
        {
            if(n>cn+1)
            {
                name=name+cn+1;
                return child->find_child_dir(name);
            }
            else
                return child;
        }

        child=child->nextEntry;
    }

    return child;
}
