#include <iostream>
#include <cstring>
#include "filedb.h"

#include "filepath.h"
#include "fileentry.h"
#include "platformutils.h"
#include "utils.h"

extern "C" {
#include <crc.h>
}

void FileDB::setSpace(uint64_t totalSpace_, uint64_t availableSpace_)
{
    totalSpace = totalSpace_;
    availableSpace = availableSpace_;
}

bool FileDB::addEntries(const FilePath &path, std::vector<std::unique_ptr<FileEntry>> entries)
{
    // Presorting entries by size so we can insert them much quicker
    std::sort(entries.begin(), entries.end(),
              [](const std::unique_ptr<FileEntry>& e1, const std::unique_ptr<FileEntry>& e2)
              {
                  return e1->get_size() > e2->get_size();
              });
    std::lock_guard<std::mutex> lock(dbMtx);
    if(!isReady() || entries.empty())
        return false;

    auto entry = _findEntry(path);
    if(entry)
    {
        for(auto& e : entries)
        {
            auto ePtr = e.get();
            ePtr->updatePathCrc(entry->pathCrc);
            auto crc = ePtr->pathCrc;
            entry->add_child(std::move(e));
            ++fileCount;

            //TODO move out of lock?
            auto it2 = entriesMap.find(crc);
            if(it2 != entriesMap.end())
                it2->second.push_back(ePtr);
            else
            {
                std::vector<FileEntry*> vec{ePtr};
                entriesMap[crc] = std::move(vec);
            }
        }
        usedSpace = rootFile->get_size();
        bHasChanges = true;
    }

    return false;
}

void FileDB::setNewRootPath(const std::string& path)
{
    std::lock_guard<std::mutex> lock(dbMtx);
    _clearDb();

    rootPath = Utils::make_unique<FilePath>(path);
    rootFile = FileEntry::createEntry(rootPath->getPath(), true);
    rootFile->updatePathCrc(0);
    fileCount = 1;
    rootValid = true;
}

const FilePath* FileDB::getRootPath() const
{
    std::lock_guard<std::mutex> lock(dbMtx);
    return rootPath.get();
}

void FileDB::clearDb()
{
    std::lock_guard<std::mutex> lock(dbMtx);
    _clearDb();
}

void FileDB::_clearDb()
{
    if(!rootValid)
        return;
    rootValid = false;
    usedSpace = 0;
    fileCount = 0;
    rootPath.reset();
    entriesMap.clear();
    FileEntry::deleteEntryChain(std::move(rootFile));
    bHasChanges = true;
}

const FileEntry* FileDB::clearEntry(const FilePath& path, uint64_t& childrenCount)
{
    std::lock_guard<std::mutex> lock_mtx(dbMtx);
    childrenCount = 0;
    if(!isReady())
        return nullptr;

    auto entry = _findEntry(path);
    if(!entry)
        return nullptr;

    Utils::tic();
    //remove all crcs for entry children

    entry->forEach([this](const FileEntry& child)->bool {
        _cleanupEntryCrc(child);
        return true;
    });

    childrenCount = entry->deleteChildren();
    if(childrenCount>=fileCount) // should not happen
        childrenCount = fileCount-1;
    fileCount -= childrenCount;
    usedSpace=rootFile->get_size();
    bHasChanges = true;

    std::cout << "Moved " <<childrenCount<< " child entries to cache\n";
    Utils::toc("Spent for clearing entry");

    return entry;
}

void FileDB::_cleanupEntryCrc(const FileEntry& entry)
{
    entry.forEach([this](const FileEntry& child)->bool {
        _cleanupEntryCrc(child);
        return true;
    });
    auto it = entriesMap.find(entry.pathCrc);
    if(it != entriesMap.end())
    {
        for(auto it2=it->second.begin(); it2 != it->second.end(); ++it2)
        {
            if((*it2) == &entry)
            {
                it->second.erase(it2);
                return;
            }
        }
    }
}

FileEntry* FileDB::_findEntry(const FilePath& path) const
{
    auto& parts = path.getParts();
    if(parts.empty())
        return nullptr;

    //provided path should have the same root as name of root entry
    if(parts.front() != rootFile->getName())
        return nullptr;
    if(parts.size() == 1)
        return rootFile.get();

    auto name = parts.back();
    if(name.back() == PlatformUtils::filePathSeparator)
        name.pop_back();

    auto pathCrc = path.getPathCrc();

    auto it = entriesMap.find(pathCrc);
    if(it == entriesMap.end())
        return nullptr;

    for(auto& possible : it->second)
    {
        auto currentEntry = possible;

        // for each possible entry check its name and names of all parents that they
        // are the same as in provided path
        for(int i=(int)parts.size()-1;i>0;--i)
        {
            auto& part = parts[i];
            bool isPartDir = part.back() == PlatformUtils::filePathSeparator;
            // part that is dir will have slash at the end so its length will be bigger by 1
            auto partLen = isPartDir ? (part.length()-1) : part.length();

            if(currentEntry->parent != nullptr // only root is without parent and we already checked it
               && strncmp(part.c_str(), currentEntry->name.get(), partLen) == 0
               && currentEntry->name[partLen] == '\0')
            {
                currentEntry = currentEntry->parent;
            } else
                break;
        }
        if(currentEntry == rootFile.get())
            return possible;
    }
    return nullptr;
}

const FileEntry* FileDB::findEntry(const FilePath& path) const
{
    std::lock_guard<std::mutex> lock_mtx(dbMtx);
    if(!isReady())
        return nullptr;
    return _findEntry(path);
}

bool FileDB::processEntry(const FilePath& path, const std::function<void(const FileEntry &)>& func) const
{
    std::lock_guard<std::mutex> lock_mtx(dbMtx);
    if(!isReady())
        return false;
    auto e = _findEntry(path);
    if(!e)
        return false;

    func(*e);

    bHasChanges = false;

    return true;
}

bool FileDB::hasChanges() const
{
    return bHasChanges;
}

bool FileDB::isReady() const
{
    return rootValid;
}

void FileDB::getSpace(uint64_t& used, uint64_t& available, uint64_t& total) const
{
    total = totalSpace;
    available = availableSpace;
    used = usedSpace;
    if(used+available>total)
        used = total - available;
}

uint64_t FileDB::getFileCount() const
{
    return fileCount;
}
