
#include <cstring>
#include <iostream>
#include "fileentrypool.h"
#include "utils.h"
#include "fileentry.h"

FileEntryPool::FileEntryPool()
{
}

std::unique_ptr<FileEntry> FileEntryPool::create_entry(const std::string& name, bool isDir)
{
    auto nameLen=name.length();
    std::unique_ptr<FileEntry> t;

    if(entriesCache.empty())
    {
        auto chars=Utils::make_unique_arr<char>(nameLen+1);
        memcpy(chars.get(), name.c_str(), (nameLen+1)*sizeof(char));

        // using manual creation because constructor of FileEntry is private
        t = std::unique_ptr<FileEntry>(new FileEntry(std::move(chars), isDir));
    }
    else
    {
        t = std::move(entriesCache.back());
        entriesCache.pop_back();
        std::unique_ptr<char[]> chars;

        //get suitable char array from cache, or create new one if no match found
        auto charsIt = charsCache.find(nameLen);
        if(charsIt != charsCache.end() && !charsIt->second.empty())
        {
            chars = std::move(charsIt->second.back());
            charsIt->second.pop_back();
        }
        else
        {
            chars = Utils::make_unique_arr<char>(nameLen+1);
        }
        memcpy(chars.get(), name.c_str(), (nameLen+1)*sizeof(char));

        t->reconstruct(std::move(chars), isDir);
    }


    return t;
}

int64_t FileEntryPool::cache_children(std::unique_ptr<FileEntry> firstChild)
{
    Utils::tic();
    auto count = _cache_children(std::move(firstChild));
    std::cout << "Moved " <<count<< " child entries to cache\n";
    Utils::toc("Spent for caching");
    return count;
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
        auto name = std::move(firstChild->name);
        firstChild->parent = nullptr;
        auto nameLen = strlen(name.get());
        entriesCache.push_back(std::move(firstChild));
        auto t = charsCache.find(nameLen);
        if(t != charsCache.end())
            t->second.push_back(std::move(name));
        else
        {
            std::vector<std::unique_ptr<char[]>> vec;
            vec.push_back(std::move(name));
            charsCache[nameLen] = std::move(vec);
        }

        firstChild = std::move(next);
    }
    return count;
}
