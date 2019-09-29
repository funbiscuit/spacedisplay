
#include <cstring>
#include <iostream>
#include <chrono>
#include "fileentrypool.h"

FileEntryPool::FileEntryPool()
{
    entryPool=new boost::object_pool<FileEntry>();
    charPool=new boost::pool<>(sizeof(char));

}

FileEntry* FileEntryPool::create_entry(uint64_t id, const char *name, FileEntry::EntryType entryType)
{
    auto nameLen=strlen(name);
    FileEntry* t;

    if(entriesCache.empty())
    {
        auto chars=(char*)charPool->ordered_malloc(nameLen+1);
        memcpy(chars, name, (nameLen+1)*sizeof(char));

        t = entryPool->construct(id, chars, entryType);
    }
    else
    {
        t = entriesCache.back();
        entriesCache.pop_back();
        char* chars;

        //get suitable char array from cache, or create new one if no match found
        auto charsIt = charsCache.find(nameLen);
        if(charsIt != charsCache.end() && !charsIt->second.empty())
        {
            chars = charsIt->second.back();
            charsIt->second.pop_back();
        }
        else
        {
            chars=(char*)charPool->ordered_malloc(nameLen+1);
        }
        memcpy(chars, name, (nameLen+1)*sizeof(char));

        t->reconstruct(id, chars, entryType);
    }


    return t;
}

void FileEntryPool::destroy_entries()
{
    entriesCache.clear();
    charsCache.clear();
    //charPool->purge_memory();
    delete(entryPool);
    delete(charPool);
    entryPool=new boost::object_pool<FileEntry>();
    charPool=new boost::pool<>(sizeof(char));
}

static int64_t files=0;

void FileEntryPool::async_destroy_children(FileEntry* firstChild)
{
    using namespace std::chrono;
    auto start   = high_resolution_clock::now();
    files=0;
    destroy_children(firstChild);

    auto stop   = high_resolution_clock::now();

    auto mseconds = duration_cast<milliseconds>(stop - start).count();

    std::cout << "Spent " << mseconds<<"ms, moving " <<files<< " child entries to cache\n";
}

void FileEntryPool::destroy_children(FileEntry* firstChild)
{
    while(firstChild != nullptr)
    {
        auto ch = firstChild->get_first_child();
        auto next = firstChild->get_next();

        if(ch)
        {
            destroy_children(firstChild->get_first_child());
        }
        ++files;

        //we don't really destroy anything since it will take a lot of time even for few thousands entries
        //instead we're moving all entries and their names to cache. next time we need to construct entry,
        //we will first look in cache to see if we have any entry and name of suitable length
        //so memory is not leaked, we are reusing it later
        entriesCache.push_back(firstChild);
        auto name = firstChild->name;
        auto t = charsCache.find(strlen(name));
        if(t != charsCache.end())
            t->second.push_back(name);
        else
        {
            std::vector<char*> vec;
            vec.push_back(name);
            charsCache[strlen(name)] = vec;
        }

        firstChild = next;
    }
}
