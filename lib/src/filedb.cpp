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

FileDB::FileDB() : bHasChanges(false), rootValid(false), usedSpace(0),
                   fileCount(0), dirCount(0)
{

}

void FileDB::setSpace(uint64_t totalSpace_, uint64_t availableSpace_)
{
    totalSpace = totalSpace_;
    availableSpace = availableSpace_;
}

bool FileDB::setChildrenForPath(const FilePath &path,
                                std::vector<std::unique_ptr<FileEntry>> entries,
                                std::vector<std::unique_ptr<FilePath>>* newPaths)
{
    if(entries.empty() || !path.isDir())
        return false;
    // Presorting entries by size so we can insert them much quicker
    std::sort(entries.begin(), entries.end(),
              [](const std::unique_ptr<FileEntry>& e1, const std::unique_ptr<FileEntry>& e2)
              {
                  return e1->getSize() > e2->getSize();
              });
    std::lock_guard<std::mutex> lock(dbMtx);
    if(!isReady())
        return false;

    auto parentEntry = _findEntry(path);
    if(!parentEntry)
        return false;

    int deletedDirCount = 0;
    int deletedFileCount = 0;
    //Mark all children for deletion
    //We will unmark every child that should be kept and then will delete everything else
    parentEntry->markChildrenPendingDelete(deletedFileCount, deletedDirCount);

    for(auto& e : entries)
    {
        auto existingChild = _findEntry(e->getName(), e->getNameCrc(), parentEntry);

        if(existingChild)
        {
            //child found, decide what to do with it. unmark it for deletion
            existingChild->unmarkPendingDelete();
            if(existingChild->isDir())
            {
                --deletedDirCount;
                // if entry is dir, just continue
                continue;
            }
            --deletedFileCount;
            if(existingChild->getSize() == e->getSize())
            {
                // if entry is file with the same size, just continue
                continue;
            }
            existingChild->setSize(e->getSize());
            continue;
        }
        // if entry was not found, we need to add it

        auto ePtr = e.get();
        parentEntry->addChild(std::move(e));
        auto crc = ePtr->getPathCrc(); //path crc will be correct after adding to parent

        if(ePtr->isDir())
            ++dirCount;
        else
            ++fileCount;

        // if directory is added, it should be put into newPaths vector so it gets scanned
        if(ePtr->isDir() && newPaths)
        {
            auto childPath = Utils::make_unique<FilePath>(path);
            childPath->addDir(ePtr->getName(), ePtr->getNameCrc());
            newPaths->push_back(std::move(childPath));
        }

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

    std::vector<std::unique_ptr<FileEntry>> deletedChildren;
    deletedChildren.reserve(deletedDirCount+deletedFileCount);

    if(deletedFileCount+deletedDirCount>0)
        parentEntry->removePendingDelete(deletedChildren);

    // also delete all pointers to removed children (and their children recursively)
    // from crc map (since all children will be deleted)
    // this function will also subtract actual deleted files and dirs from fileCount and dirCount
    // actual deleted number might be different from deletedFileCount and deletedDirCount since
    // deleted directories might have children too
    for(auto& child : deletedChildren)
        _cleanupEntryCrc(*child);

    usedSpace = rootFile->getSize();
    bHasChanges = true;

    return true;
}

void FileDB::setNewRootPath(const std::string& path)
{
    std::lock_guard<std::mutex> lock(dbMtx);

    // clearing db resets rootPath so store to temp var (constructor of FilePath can throw exception if path is empty)
    auto newRootPath = Utils::make_unique<FilePath>(path);
    _clearDb();
    rootPath = std::move(newRootPath);
    rootFile = Utils::make_unique<FileEntry>(rootPath->getPath(), true);
    dirCount = 1;
    fileCount = 0;
    rootValid = true;
    bHasChanges = true;
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
    dirCount = 0;
    rootPath.reset();
    entriesMap.clear();
    rootFile.reset();
    bHasChanges = true;
}

void FileDB::_cleanupEntryCrc(const FileEntry& entry)
{
    entry.forEach([this](const FileEntry& child)->bool {
        _cleanupEntryCrc(child);
        return true;
    });
    auto it = entriesMap.find(entry.getPathCrc());
    if(it != entriesMap.end())
    {
        for(auto it2=it->second.begin(); it2 != it->second.end(); ++it2)
        {
            if((*it2) == &entry)
            {
                if(entry.isDir())
                    --dirCount;
                else
                    --fileCount;
                it->second.erase(it2);
                if(it->second.empty())
                    entriesMap.erase(it);
                return;
            }
        }
    }
}

FileEntry* FileDB::_findEntry(const char* entryName, uint16_t nameCrc, FileEntry* parent) const
{
    if(!parent)
    {
        //only root can be without parents
        if(strcmp(entryName, rootFile->getName()) == 0)
            return rootFile.get();
        return nullptr;
    }

    auto pathCrc = parent->getPathCrc() ^ nameCrc;
    auto it = entriesMap.find(pathCrc);
    if(it == entriesMap.end())
        return nullptr;

    auto vIt = it->second.begin();
    while(vIt != it->second.end())
    {
        if((*vIt)->getParent() == parent && strcmp(entryName, (*vIt)->getName()) == 0)
            return (*vIt);
        ++vIt;
    }
    return nullptr;
}

FileEntry* FileDB::_findEntry(const FilePath& path) const
{
    auto& parts = path.getParts();

    //provided path should have the same root as name of root entry
    if(parts.front() != rootFile->getName())
        return nullptr;
    if(parts.size() == 1)
        return rootFile.get();

    auto pathCrc = path.getPathCrc();

    auto it = entriesMap.find(pathCrc);
    if(it == entriesMap.end())
        return nullptr;

    auto vIt = it->second.begin();

    while(vIt != it->second.end())
    {
        const FileEntry* currentEntry = *vIt;

        // for each possible entry check its name and names of all parents that they
        // are the same as in provided path
        for(int i=(int)parts.size()-1;i>0;--i)
        {
            auto& part = parts[i];
            bool isPartDir = part.back() == PlatformUtils::filePathSeparator;
            // part that is dir will have slash at the end so its length will be bigger by 1
            auto partLen = isPartDir ? (part.length()-1) : part.length();

            auto name = currentEntry->getName();

            if(currentEntry->getParent() != nullptr // only root is without parent and we already checked it
               && strncmp(part.c_str(), name, partLen) == 0
               && name[partLen] == '\0')
            {
                currentEntry = currentEntry->getParent();
            } else
                break;
        }
        if(currentEntry == rootFile.get())
            return *vIt;

        ++vIt;
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

uint64_t FileDB::getDirCount() const
{
    return dirCount;
}
