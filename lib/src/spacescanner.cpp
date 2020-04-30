
#include "spacescanner.h"
#include "fileentry.h"
#include "filepath.h"
#include "filedb.h"
#include "platformutils.h"
#include "utils.h"

#include <iostream>
#include <chrono>

SpaceScanner::SpaceScanner() :
        scannerStatus(ScannerStatus::IDLE), runWorker(false), isMountScanned(false)
{
    db = std::make_shared<FileDB>();
    //Start thread after everything is initialized
    workerThread=std::thread(&SpaceScanner::worker_run, this);
}

SpaceScanner::~SpaceScanner()
{
    runWorker = false;
    scannerStatus=ScannerStatus::STOPPING;
    workerThread.join();
    db->clearDb();
}

void SpaceScanner::worker_run()
{
    std::cout << "Start worker thread\n";
    runWorker = true;
    while(runWorker)
    {
        std::unique_lock<std::mutex> scanLock(scanMtx, std::defer_lock);
        scanLock.lock();

        using namespace std::chrono;
        while(scanQueue.empty() && scannerStatus==ScannerStatus::IDLE)
        {
            scanLock.unlock();
            std::this_thread::sleep_for(milliseconds(20));
            scanLock.lock();
        }

        //scan queue is not empty at this point
        auto start = high_resolution_clock::now();
        // temporary queue for entries so we only lock mutex to add reasonable number of entries
        // instead of locking for each entry
        std::vector<std::unique_ptr<FileEntry>> scannedEntries;

        while(!scanQueue.empty() && scannerStatus==ScannerStatus::SCANNING)
        {
            auto entryPath = std::move(scanQueue.front());
            scanQueue.pop_front();
            // update current scanned path with new data
            if(currentScannedPath)
                *currentScannedPath = *entryPath;
            else
                currentScannedPath = Utils::make_unique<FilePath>(*entryPath);
            scanLock.unlock();

            scanChildrenAt(*entryPath, scannedEntries);
            // after scan, all new paths, that should be scanned, are already added to queue
            // so we should add all new entries to database, otherwise we might not be able
            // to find them on the next iteration

            if(scannerStatus==ScannerStatus::STOPPING)
            {
                //lock mutex because next action will be clearing of queue and it should be protected
                scanLock.lock();
                break;
            }

            db->setChildrenForPath(*entryPath, std::move(scannedEntries));

            while(scannerStatus==ScannerStatus::SCAN_PAUSED)
            {
                //if scan is paused, just wait until it isn't
                std::this_thread::sleep_for(milliseconds(20));
            }

            scanLock.lock();
        }
        scanQueue.clear();
        currentScannedPath = nullptr;
        scanLock.unlock();

        auto stop   = high_resolution_clock::now();
        auto mseconds = duration_cast<milliseconds>(stop - start).count();
        std::cout << "Time taken: " << mseconds <<"ms, "<<db->getFileCount()<<" file(s) scanned\n";
        scannerStatus = ScannerStatus::IDLE;
    }

    std::cout << "End worker thread\n";
}

void SpaceScanner::scanChildrenAt(const FilePath& path, std::vector<std::unique_ptr<FileEntry>>& scannedEntries)
{
    auto pathStr = path.getPath();

    //TODO add check if iterator was constructed and we were able to open path
    for(FileIterator it(pathStr); it.is_valid(); ++it)
    {
        bool doScan = it.isDir;
        std::unique_ptr<FilePath> entryPath;
        // this section is important for linux since not any path should be scanned (e.g. /proc or /sys)
        if(it.isDir && scannerStatus != ScannerStatus::STOPPING)
        {
            std::string newPath = pathStr;
            if(newPath.back() != PlatformUtils::filePathSeparator)
                newPath.push_back(PlatformUtils::filePathSeparator);
            newPath.append(it.name);
            if(newPath.back() != PlatformUtils::filePathSeparator)
                newPath.push_back(PlatformUtils::filePathSeparator);
            if(Utils::in_array(newPath, availableRoots) || Utils::in_array(newPath, excludedMounts))
            {
                doScan = false;
                std::cout<<"Skip scan of: "<<newPath<<"\n";
            }
        }
        auto fe=FileEntry::createEntry(it.name, it.isDir);
        fe->set_size(it.size);
        if(doScan)
        {
            entryPath = Utils::make_unique<FilePath>(path);
            entryPath->addDir(it.name, fe->getNameCrc16());
            std::lock_guard<std::mutex> lock(scanMtx);
            //TODO it would be faster to save all adding for later and then just check if
            // parent path exist in entry (or any of its child paths)
            // if it is not (and most of the times this will be true), we can just push all
            // these paths without further checking
            // but this will give a small improvement (around 1sec when scanning C:\\
            // which takes 15 seconds to scan)
            addToQueue(std::move(entryPath), false);
        }
        scannedEntries.push_back(std::move(fe));
    }
}

void SpaceScanner::addToQueue(std::unique_ptr<FilePath> path, bool toBack)
{
    auto it = scanQueue.begin();

    while(it != scanQueue.end())
    {
        auto res = path->compareTo(*(*it));

        if(res == FilePath::CompareResult::DIFFERENT || res == FilePath::CompareResult::CHILD)
            ++it;
        else if(res == FilePath::CompareResult::PARENT)
            //if we are parent path to some path in queue, then we should remove it from queue
            it = scanQueue.erase(it);
        else //equal
        {
            scanQueue.erase(it);
            break;
        }
    }

    if(toBack)
        scanQueue.push_back(std::move(path));
    else
        scanQueue.push_front(std::move(path));
}

std::vector<std::string> SpaceScanner::get_available_roots()
{
    PlatformUtils::get_mount_points(availableRoots, excludedMounts);

    return availableRoots;
}

std::shared_ptr<FileDB> SpaceScanner::getFileDB()
{
    return db;
}

bool SpaceScanner::getCurrentScanPath(std::unique_ptr<FilePath>& path)
{
    std::lock_guard<std::mutex> lock_mtx(scanMtx);

    if(currentScannedPath)
    {
        if(!path)
            path = Utils::make_unique<FilePath>(*currentScannedPath);
        else
            *path = *currentScannedPath;
        return true;
    }
    path = nullptr;
    return false;
}

bool SpaceScanner::can_refresh() const
{
    return db->isReady();
}

bool SpaceScanner::is_loaded() const
{
    return db->isReady();
}

bool SpaceScanner::has_changes() const
{
    return db->hasChanges();
}

bool SpaceScanner::isProgressKnown() const
{
    return isMountScanned;
}

int SpaceScanner::get_scan_progress() const
{
    uint64_t totalSpace, usedSpace, freeSpace;
    db->getSpace(usedSpace, freeSpace, totalSpace);
    if((scannerStatus==ScannerStatus::SCANNING || scannerStatus==ScannerStatus::SCAN_PAUSED)
       && totalSpace>0)
    {
        int progress = static_cast<int>((usedSpace*100)/(totalSpace-freeSpace));
        return Utils::clip(progress, 0, 100);
    }
    return 100;
}

bool SpaceScanner::pauseScan()
{
    std::lock_guard<std::mutex> lock_mtx(scanMtx);
    if(scannerStatus==ScannerStatus::SCANNING)
    {
        scannerStatus=ScannerStatus::SCAN_PAUSED;
        return true;
    }
    return false;
}

bool SpaceScanner::resumeScan()
{
    std::lock_guard<std::mutex> lock_mtx(scanMtx);
    if(scannerStatus==ScannerStatus::SCAN_PAUSED)
    {
        scannerStatus=ScannerStatus::SCANNING;
        return true;
    }
    return false;
}

bool SpaceScanner::canPause()
{
    return scannerStatus == ScannerStatus::SCANNING;
}

bool SpaceScanner::canResume()
{
    return scannerStatus == ScannerStatus::SCAN_PAUSED;
}

void SpaceScanner::stop_scan()
{
    {
        std::lock_guard<std::mutex> lock_mtx(scanMtx);
        if(scannerStatus!=ScannerStatus::STOPPING && scannerStatus!=ScannerStatus::IDLE)
            scannerStatus=ScannerStatus::STOPPING;
    }
    //wait until everything is stopped
    while(scannerStatus!=ScannerStatus::IDLE)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void SpaceScanner::update_disk_space()
{
    auto p = db->getRootPath();
    if(!p)
        return;

    uint64_t total, available;

    PlatformUtils::get_mount_space(p->getName(), total, available);
    db->setSpace(total, available);
}

ScannerError SpaceScanner::scan_dir(const std::string &path)
{
    //do nothing if scan is already in progress
    if(scannerStatus!=ScannerStatus::IDLE)
        return ScannerError::SCAN_RUNNING;

    if(!PlatformUtils::can_scan_dir(path))
    {
        scannerStatus=ScannerStatus::IDLE;
        std::cout<<"Can't open "<<path<<"\n";
        return ScannerError::CANT_OPEN_DIR;
    }

    std::lock_guard<std::mutex> lock_mtx(scanMtx);
    //update info about mount points
    PlatformUtils::get_mount_points(availableRoots, excludedMounts);
    // clears database if it was populated and sets new root
    db->setNewRootPath(path);

    scannerStatus=ScannerStatus::SCANNING;

    isMountScanned = false;
    for(const auto& root : availableRoots)
    {
        if(root == path)
        {
            isMountScanned = true;
            break;
        }
    }

    //this will load known info about disk space (available and total) to database
    update_disk_space();

    addToQueue(Utils::make_unique<FilePath>(*(db->getRootPath())));

    return ScannerError::NONE;
}

void SpaceScanner::rescan_dir(const FilePath& path)
{
    std::lock_guard<std::mutex> lock_mtx(scanMtx);
    auto entry = db->findEntry(path);
    
    if(!entry)
        return;

    scannerStatus=ScannerStatus::SCANNING;
    //update info about mount points
    PlatformUtils::get_mount_points(availableRoots, excludedMounts);

    update_disk_space();//disk space might change since last update, so update it again

    // pushing to front so we start rescanning as soon as possible
    addToQueue(Utils::make_unique<FilePath>(path), false);
}

void SpaceScanner::getSpace(uint64_t& used, uint64_t& available, uint64_t& total) const
{
    db->getSpace(used, available, total);
}

int64_t SpaceScanner::getFileCount() const
{
    return db->getFileCount();
}

const FilePath* SpaceScanner::getRootPath() const
{
    return db->getRootPath();
}
