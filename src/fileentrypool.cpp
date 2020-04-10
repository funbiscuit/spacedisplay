
#include <cstring>
#include <iostream>
#include "fileentrypool.h"
#include "utils.h"

FileEntryPool::FileEntryPool()
{
}

std::unique_ptr<FileEntry> FileEntryPool::create_entry(uint64_t id, const char *name, FileEntry::EntryType entryType)
{
    auto nameLen=strlen(name);
    std::unique_ptr<FileEntry> t;

    if(entriesCache.empty())
    {
        auto chars=new char[nameLen+1];
        memcpy(chars, name, (nameLen+1)*sizeof(char));

        t = Utils::make_unique<FileEntry>(id, chars, entryType);
    }
    else
    {
        t = std::move(entriesCache.back());
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

void FileEntryPool::cache_children(std::unique_ptr<FileEntry> firstChild)
{
    Utils::tic();
    auto count = _cache_children(std::move(firstChild));
    std::cout << "Moved " <<count<< " child entries to cache\n";
    Utils::toc("Spent for caching");
}

void FileEntryPool::delete_children(std::unique_ptr<FileEntry> firstChild)
{
    //don't delete, use caching
    //TODO delete may be okay on exit

    Utils::tic();
    auto count = _delete_children(std::move(firstChild));
    std::cout << "Deleted " <<count<< " child entries\n";
    Utils::toc("Spent for deleting");
}

void FileEntryPool::cleanup_cache()
{
    //don't delete, use caching
    //TODO delete may be okay on exit

    Utils::tic();
    //unique_ptr's will auto destroy
    entriesCache.clear();

    for(auto& it : charsCache)
        for(auto& chars : it.second)
            delete[](chars);

    charsCache.clear();

    Utils::toc("Spent for deleting cache");
}

uint64_t FileEntryPool::_delete_children(std::unique_ptr<FileEntry> firstChild)
{
    uint64_t count = 0;
    while(firstChild)
    {
        auto ch = firstChild->pop_children();
        auto next = firstChild->pop_next();

        if(ch)
            count += _delete_children(std::move(ch));

        ++count;
        auto name = firstChild->name;
        delete[](name);

        firstChild = std::move(next);
    }
    return count;
}

uint64_t FileEntryPool::_cache_children(std::unique_ptr<FileEntry> firstChild)
{
    uint64_t count = 0;
    while(firstChild)
    {
        auto ch = firstChild->pop_children();
        auto next = firstChild->pop_next();

        if(ch)
            count += _cache_children(std::move(ch));

        //we don't really destroy anything since it will take a lot of time even for few thousands entries
        //instead we're moving all entries and their names to cache. next time we need to construct entry,
        //we will first look in cache to see if we have any entry and name of suitable length
        //so memory is not leaked, we are reusing it later
        ++count;
        auto name = firstChild->name;
        entriesCache.push_back(std::move(firstChild));
        auto t = charsCache.find(strlen(name));
        if(t != charsCache.end())
            t->second.push_back(name);
        else
        {
            std::vector<char*> vec;
            vec.push_back(name);
            charsCache[strlen(name)] = vec;
        }

        firstChild = std::move(next);
    }
    return count;
}
