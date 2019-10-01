
#include <cstring>
#include <iostream>
#include "fileentrypool.h"
#include "utils.h"

FileEntryPool::FileEntryPool()
{
}

FileEntry* FileEntryPool::create_entry(uint64_t id, const char *name, FileEntry::EntryType entryType)
{
    auto nameLen=strlen(name);
    FileEntry* t;

    if(entriesCache.empty())
    {
        auto chars=new char[nameLen+1];
        memcpy(chars, name, (nameLen+1)*sizeof(char));

        t = new FileEntry(id, chars, entryType);
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
            chars=new char[nameLen+1];
        }
        memcpy(chars, name, (nameLen+1)*sizeof(char));

        t->reconstruct(id, chars, entryType);
    }


    return t;
}

void FileEntryPool::cache_children(FileEntry* firstChild)
{
    Utils::tic();
    auto count = _cache_children(firstChild);
    std::cout << "Moved " <<count<< " child entries to cache\n";
    Utils::toc("Spent for caching");
}

void FileEntryPool::delete_children(FileEntry* firstChild)
{
    //don't delete, use caching
    //TODO delete may be okay on exit

    Utils::tic();
    auto count = _delete_children(firstChild);
    std::cout << "Deleted " <<count<< " child entries\n";
    Utils::toc("Spent for deleting");
}

void FileEntryPool::cleanup_cache()
{
    //don't delete, use caching
    //TODO delete may be okay on exit

    Utils::tic();
    for(auto& entry : entriesCache)
        delete(entry);
    entriesCache.clear();

    for(auto& it : charsCache)
        for(auto& chars : it.second)
            delete[](chars);

    charsCache.clear();

    Utils::toc("Spent for deleting cache");
}

uint64_t FileEntryPool::_delete_children(FileEntry* firstChild)
{
    uint64_t count = 0;
    while(firstChild != nullptr)
    {
        auto ch = firstChild->get_first_child();
        auto next = firstChild->get_next();

        if(ch)
            count += _delete_children(firstChild->get_first_child());

        ++count;
        auto name = firstChild->name;
        delete(firstChild);
        delete[](name);

        firstChild = next;
    }
    return count;
}

uint64_t FileEntryPool::_cache_children(FileEntry* firstChild)
{
    uint64_t count = 0;
    while(firstChild != nullptr)
    {
        auto ch = firstChild->get_first_child();
        auto next = firstChild->get_next();

        if(ch)
            count += _cache_children(firstChild->get_first_child());

        //we don't really destroy anything since it will take a lot of time even for few thousands entries
        //instead we're moving all entries and their names to cache. next time we need to construct entry,
        //we will first look in cache to see if we have any entry and name of suitable length
        //so memory is not leaked, we are reusing it later
        ++count;
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
    return count;
}
