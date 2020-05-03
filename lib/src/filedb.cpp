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

bool FileDB::setChildrenForPath(const FilePath &path,
                                std::vector<std::unique_ptr<FileEntry>> entries,
                                std::vector<std::unique_ptr<FilePath>>* newPaths)
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

    auto parentEntry = _findEntry(path);
    if(parentEntry)
    {
        int deletedDirCount = 0;
        int deletedFileCount = 0;
        //Mark all children for deletion
        //We will unmark every child that should be kept and then will delete everything else
        for(auto& childBins : parentEntry->children)
        {
            auto child = childBins.firstEntry.get();
            while(child)
            {
                if(child->isDir)
                    ++deletedDirCount;
                else
                    ++deletedFileCount;
                child->pendingDelete = true;
                child = child->nextEntry.get();
            }
        }


        for(auto& e : entries)
        {
            auto existingChild = _findEntry(e->name.get(), e->nameCrc, parentEntry);

            if(existingChild)
            {
                //child found, decide what to do with it. unmark it for deletion
                existingChild->pendingDelete = false;
                if(existingChild->isDir)
                {
                    --deletedDirCount;
                    // if entry is dir or file with the same size, just continue
                    continue;
                }
                --deletedFileCount;
                if(existingChild->size == e->size)
                {
                    // if entry is dir or file with the same size, just continue
                    continue;
                }
                auto sizeChange = e->size-existingChild->size;
                // if size changed, tell about it to parent, but we need to increase child size by ourself
                existingChild->size = e->size;
                parentEntry->on_child_size_changed(existingChild, sizeChange);
                continue;
            }
            // if entry was not found, we need to add it

            auto ePtr = e.get();
            ePtr->updatePathCrc(parentEntry->pathCrc);
            auto crc = ePtr->pathCrc;
            parentEntry->add_child(std::move(e));

            if(ePtr->isDir)
                ++dirCount;
            else
                ++fileCount;

            // if directory is added, it should be put into newPaths vector so it gets scanned
            if(ePtr->isDir && newPaths)
            {
                auto childPath = Utils::make_unique<FilePath>(path);
                childPath->addDir(ePtr->name.get(), ePtr->nameCrc);
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
        fileCount-=deletedFileCount;
        dirCount-=deletedDirCount;

        //also delete all pointers to this children from crc map (since all children will be deleted)
        for(auto& child : deletedChildren)
        {
            auto crc = child->pathCrc;
            //decrease counters back. after exiting loop they should be zero, otherwise not all children
            //were actually deleted and we will need to fix fileCount and dirCount
            if(child->isDir)
                --deletedDirCount;
            else
                --deletedFileCount;

            auto it2 = entriesMap.find(crc);
            if(it2 != entriesMap.end())
            {
                auto it3 = it2->second.begin();
                while(it3 != it2->second.end())
                {
                    if((*it3) == child.get())
                    {
                        it2->second.erase(it3);
                        break;
                    }
                    ++it3;
                }
            }
        }
        fileCount+=deletedFileCount;
        dirCount+=deletedDirCount;

        usedSpace = rootFile->get_size();
        bHasChanges = true;
    }

    return true;
}

void FileDB::setNewRootPath(const std::string& path)
{
    std::lock_guard<std::mutex> lock(dbMtx);
    _clearDb();

    rootPath = Utils::make_unique<FilePath>(path);
    rootFile = FileEntry::createEntry(rootPath->getPath(), true);
    rootFile->updatePathCrc(0);
    dirCount = 1;
    fileCount = 0;
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

FileEntry* FileDB::_findEntry(const char* entryName, uint16_t nameCrc, FileEntry* parent) const
{
    if(!parent)
    {
        //only root can be without parents
        if(strcmp(entryName, rootFile->name.get()) == 0)
            return rootFile.get();
        return nullptr;
    }

    auto pathCrc = parent->pathCrc ^ nameCrc;
    auto it = entriesMap.find(pathCrc);
    if(it == entriesMap.end())
        return nullptr;

    auto vIt = it->second.begin();
    while(vIt != it->second.end())
    {
        if((*vIt)->parent == parent && strcmp(entryName, (*vIt)->name.get()) == 0)
            return (*vIt);
        ++vIt;
    }
    return nullptr;
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

    auto vIt = it->second.begin();

    while(vIt != it->second.end())
    {
        auto currentEntry = *vIt;

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
