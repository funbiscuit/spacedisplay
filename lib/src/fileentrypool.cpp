
#include <cstring>
#include <iostream>
#include "fileentrypool.h"
#include "utils.h"
#include "fileentry.h"

FileEntryPool::FileEntryPool()
{
}
FileEntryPool::~FileEntryPool()
{
    //TODO check if we should call this on exit
    std::cout << "Cleaning up cache:\n";
    cleanup_cache();
}

std::unique_ptr<FileEntry> FileEntryPool::create_entry(const std::string& name, bool isDir)
{
    auto nameLen=name.length();
    std::unique_ptr<FileEntry> t;

    std::unique_lock<std::mutex> lock(cacheMtx);
    if(entriesCache.empty())
    {
        lock.unlock();
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
        lock.unlock();
        memcpy(chars.get(), name.c_str(), (nameLen+1)*sizeof(char));

        t->reconstruct(std::move(chars), isDir);
    }


    return t;
}

int64_t FileEntryPool::cache_children(std::unique_ptr<FileEntry> firstChild)
{
    return _cache_children(std::move(firstChild));
}

void FileEntryPool::cleanup_cache()
{
    std::lock_guard<std::mutex> lock(cacheMtx);
    //don't delete, use caching
    //TODO delete may be okay on exit

    Utils::tic();
    //unique_ptr's will auto destroy
    entriesCache.clear();
    charsCache.clear();

    Utils::toc("Spent for deleting cache");
}


uint64_t FileEntryPool::_cache_children(std::unique_ptr<FileEntry> firstChild)
{
    std::unique_lock<std::mutex> lock(cacheMtx, std::defer_lock);
    uint64_t count = 0;
    while(firstChild)
    {
        firstChild->parent = nullptr;
        auto next = std::move(firstChild->nextEntry);
        count += firstChild->deleteChildren();

        //we don't really destroy anything since it will take a lot of time even for few thousands entries
        //instead we're moving all entries and their names to cache. next time we need to construct entry,
        //we will first look in cache to see if we have any entry and name of suitable length
        //so memory is not leaked, we are reusing it later
        ++count;
        auto name = std::move(firstChild->name);
        firstChild->parent = nullptr;
        auto nameLen = strlen(name.get());
        lock.lock();
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
        lock.unlock();

        firstChild = std::move(next);
    }
    return count;
}
