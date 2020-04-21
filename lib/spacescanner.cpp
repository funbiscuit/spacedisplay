
#include "spacescanner.h"
#include "fileentry.h"
#include "filepath.h"
#include "filedb.h"
#include "platformutils.h"
#include "utils.h"

#include <iostream>
#include <chrono>

SpaceScanner::SpaceScanner() :
        scannerStatus(ScannerStatus::IDLE), runWorker(false)
{
    db = std::make_shared<FileDB>();
    //Start thread after everything is initialized
    workerThread=std::thread(&SpaceScanner::worker_run, this);
}

SpaceScanner::~SpaceScanner()
{
    runWorker = false;
    workerThread.join();
    db->clearDb();
    //TODO check if we should call this on exit
//    std::cout << "Cleaning up cache:\n";
//    entryPool->cleanup_cache();
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
        while(scanQueue.empty() && runWorker && scannerStatus==ScannerStatus::IDLE)
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

        while(!scanQueue.empty() && runWorker && scannerStatus==ScannerStatus::SCANNING)
        {
            auto entryPath = std::move(scanQueue.front());
            scanQueue.pop_front();
            scanLock.unlock();

            scanChildrenAt(*entryPath, scannedEntries);
            // after scan, all new paths, that should be scanned, are already added to queue
            // so we should add all new entries to database, otherwise we might not be able
            // to find them on the next iteration

            if(scannerStatus!=ScannerStatus::SCANNING)
            {
                //lock mutex because next action will be clearing of queue and it should be protected
                scanLock.lock();
                break;
            }

            db->addEntries(*entryPath, std::move(scannedEntries));

            scanLock.lock();
        }
        scanQueue.clear();
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
        bool addToQueue = it.isDir;
        std::unique_ptr<FilePath> entryPath;
        // this section is important for linux since not any path should be scanned (e.g. /proc or /sys)
        if(it.isDir && scannerStatus == ScannerStatus::SCANNING)
        {
            std::string newPath = pathStr;
            if(newPath.back() != PlatformUtils::filePathSeparator)
                newPath.push_back(PlatformUtils::filePathSeparator);
            newPath.append(it.name);
            if(newPath.back() != PlatformUtils::filePathSeparator)
                newPath.push_back(PlatformUtils::filePathSeparator);
            if(Utils::in_array(newPath, availableRoots) || Utils::in_array(newPath, excludedMounts))
            {
                addToQueue = false;
                std::cout<<"Skip scan of: "<<newPath<<"\n";
            }
        }
        auto fe=FileEntry::createEntry(it.name, it.isDir);
        fe->set_size(it.size);
        if(addToQueue)
        {
            entryPath = Utils::make_unique<FilePath>(path);
            entryPath->addDir(it.name, fe->getNameCrc16());
            scanQueue.push_front(std::move(entryPath));
        }
        scannedEntries.push_back(std::move(fe));
    }
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

bool SpaceScanner::can_refresh()
{
    return db->isReady() && !is_running();
}

bool SpaceScanner::is_running()
{
    return scannerStatus!=ScannerStatus::IDLE;
}

bool SpaceScanner::is_loaded()
{
    return db->isReady();
}

bool SpaceScanner::has_changes()
{
    return db->hasChanges();
}

float SpaceScanner::get_scan_progress()
{
    uint64_t totalSpace, usedSpace, freeSpace;
    db->getSpace(usedSpace, freeSpace, totalSpace);
    if(scannerStatus==ScannerStatus::SCANNING && totalSpace>0)
        return float(usedSpace)/float(totalSpace-freeSpace);
    return 1.f;
}

void SpaceScanner::stop_scan()
{
    {
        std::lock_guard<std::mutex> lock_mtx(scanMtx);
        if(scannerStatus==ScannerStatus::SCANNING)
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
    // clears database if it was populated and sets new root
    db->setNewRootPath(path);

    scannerStatus=ScannerStatus::SCANNING;

    //this will load known info about disk space (available and total) to database
    update_disk_space();

    scanQueue.push_back(Utils::make_unique<FilePath>(*(db->getRootPath())));

    return ScannerError::NONE;
}

void SpaceScanner::rescan_dir(const FilePath& path)
{
    std::lock_guard<std::mutex> lock_mtx(scanMtx);
    auto entry = db->findEntry(path);
    
    if(!entry)
        return;

    scannerStatus=ScannerStatus::SCANNING;

    update_disk_space();//disk space might change since last update, so update it again

    uint64_t count;
    if(!db->clearEntry(path, count))
    {
        std::cout<<"Can't rescan "<<path.getPath()<<"\n";
        return;
    }

    scanQueue.push_back(Utils::make_unique<FilePath>(path));
}

void SpaceScanner::getSpace(uint64_t& used, uint64_t& available, uint64_t& total) const
{
    db->getSpace(used, available, total);
}

const FilePath* SpaceScanner::getRootPath() const
{
    return db->getRootPath();
}
