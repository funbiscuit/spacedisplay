
#include <cstring>
#include <iostream>
#include <chrono>
#include "fileentrypool.h"
#include "utils.h"

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
    Utils::tic();
    entriesCache.clear();
    charsCache.clear();
    //charPool->purge_memory();
    delete(entryPool);
    delete(charPool);
    entryPool=new boost::object_pool<FileEntry>();
    charPool=new boost::pool<>(sizeof(char));
    Utils::toc("Destroyed entries");
}

void FileEntryPool::cache_children(FileEntry* firstChild)
{
    Utils::tic();
    int64_t before=entriesCache.size();
    _cache_children(firstChild);
    std::cout << "Moved " <<entriesCache.size()-before<< " child entries to cache\n";
    Utils::toc("Spent for moving");
}

void FileEntryPool::_cache_children(FileEntry* firstChild)
{
    while(firstChild != nullptr)
    {
        auto ch = firstChild->get_first_child();
        auto next = firstChild->get_next();

        if(ch)
            _cache_children(firstChild->get_first_child());

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
