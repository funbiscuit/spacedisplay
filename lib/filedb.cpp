#include <iostream>
#include "filedb.h"

#include "filepath.h"
#include "fileentry.h"
#include "utils.h"

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

    auto entry = rootFile->findEntry(&path);
    if(entry)
    {
        for(auto& e : entries)
        {
            entry->add_child(std::move(e));
            ++fileCount;
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
    FileEntry::deleteEntryChain(std::move(rootFile));
    bHasChanges = true;
}

const FileEntry* FileDB::clearEntry(const FilePath& path, uint64_t& childrenCount)
{
    std::lock_guard<std::mutex> lock_mtx(dbMtx);
    childrenCount = 0;
    if(!isReady())
        return nullptr;

    auto entry = rootFile->findEntry(&path);
    if(!entry)
        return nullptr;

    childrenCount = entry->deleteChildren();
    if(childrenCount>=fileCount) // should not happen
        childrenCount = fileCount-1;
    fileCount -= childrenCount;
    usedSpace=rootFile->get_size();
    bHasChanges = true;

    return entry;

}

const FileEntry* FileDB::findEntry(const FilePath& path) const
{
    std::lock_guard<std::mutex> lock_mtx(dbMtx);
    return isReady() ? rootFile->findEntry(&path) : nullptr;
}

bool FileDB::processEntry(const FilePath& path, const std::function<void(const FileEntry &)>& func) const
{
    std::lock_guard<std::mutex> lock_mtx(dbMtx);
    if(!isReady())
        return false;
    auto e = rootFile->findEntry(&path);
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
