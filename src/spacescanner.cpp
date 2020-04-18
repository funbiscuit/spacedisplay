
#include "spacescanner.h"
#include "fileentry.h"
#include "filepath.h"
#include "fileentryview.h"
#include "fileentrypool.h"
#include "platformutils.h"

#include <fstream>
#include <iostream>
#include <chrono>

SpaceScanner::SpaceScanner() :
        scannerStatus(ScannerStatus::IDLE), rootAvailable(false)
{
    entryPool = Utils::make_unique<FileEntryPool>();
    //Start thread after everything is initialized
    workerThread=std::thread(&SpaceScanner::worker_run, this);
}

SpaceScanner::~SpaceScanner()
{
    runWorker = false;
    workerThread.join();
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
        std::unique_lock<std::mutex> dbLock(mtx, std::defer_lock);
        dbLock.lock();

        using namespace std::chrono;
        while(scanQueue.empty() && runWorker && scannerStatus==ScannerStatus::IDLE)
        {
            dbLock.unlock();
            std::this_thread::sleep_for(milliseconds(20));
            dbLock.lock();
        }

        //scan queue is not empy at this point
        auto start = high_resolution_clock::now();
        // temporary queue for entries so we only lock mutex to add reasonable number of entries
        // instead of locking for each entry
        std::vector<ScannedEntry> scannedEntries;
        while(!scanQueue.empty() && runWorker && scannerStatus==ScannerStatus::SCANNING)
        {
            auto entry = scanQueue.front();
            scanQueue.pop_front();
            dbLock.unlock();

            update_entry_children(entry, scannedEntries);

            dbLock.lock();

            //just an arbitrary number so we push entries to tree only when we have enough of them
            if(scannedEntries.size()<100 && !scanQueue.empty())
                continue;

            if(scannerStatus!=ScannerStatus::SCANNING)
                break;

            auto prevCount = fileCount;
            //don't spend too much time here for adding entries since GUI thread might be waiting for new data
            while(!scannedEntries.empty() && (fileCount-prevCount<200))
            {
                auto p = std::move(scannedEntries.back());
                scannedEntries.pop_back();

                if(p.addToQueue)
                    scanQueue.push_front(p.entry.get());
                p.parent->add_child(std::move(p.entry));
                ++fileCount;
                hasPendingChanges = true;
            }

            scannedSpace = rootFile->get_size();
        }
        dbLock.unlock();

        auto stop   = high_resolution_clock::now();
        auto mseconds = duration_cast<milliseconds>(stop - start).count();
        std::cout << "Time taken: " << mseconds <<"ms, "<<fileCount<<" file(s) scanned\n";
        scannerStatus = ScannerStatus::IDLE;
    }

    std::cout << "End worker thread\n";
}

void SpaceScanner::update_entry_children(FileEntry* entry, std::vector<ScannedEntry>& scannedEntries)
{
    std::string path;
    entry->get_path(path);

    //TODO add check if iterator was constructed and we were able to open path
    for(FileIterator it(path); it.is_valid(); ++it)
    {
        auto fe=entryPool->create_entry(it.name, it.isDir);
        fe->set_size(it.size);

        bool addToQueue = it.isDir;
        // this section is important for linux since not any path should be scanned (e.g. /proc or /sys)
        if(it.isDir && scannerStatus == ScannerStatus::SCANNING)
        {
            std::string newPath;
            fe->get_path(newPath);
            if(newPath.back() != PlatformUtils::filePathSeparator)
                newPath.push_back(PlatformUtils::filePathSeparator);
            if(Utils::in_array(newPath, availableRoots) || Utils::in_array(newPath, excludedMounts))
            {
                addToQueue = false;
                std::cout<<"Skip scan of: "<<newPath<<"\n";
            }
        }
        scannedEntries.push_back({std::move(fe), addToQueue, entry});
    }
}

std::vector<std::string> SpaceScanner::get_available_roots()
{
    PlatformUtils::get_mount_points(availableRoots, excludedMounts);

    return availableRoots;
}

void SpaceScanner::reset_database()
{
    stop_scan();
    std::lock_guard<std::mutex> lock_mtx(mtx);
    fileCount=0;
    scannedSpace = 0;
    entryPool->cache_children(std::move(rootFile));
    rootAvailable=false;
}

bool SpaceScanner::can_refresh()
{
    return rootAvailable && !is_running();
}

bool SpaceScanner::is_running()
{
    return scannerStatus!=ScannerStatus::IDLE;
}

bool SpaceScanner::is_loaded()
{
    return rootAvailable;
}

bool SpaceScanner::has_changes()
{
    return hasPendingChanges;
}

void SpaceScanner::updateEntryView(FileEntryViewPtr& view, float minSizeRatio, uint16_t flags, const FilePath* filepath, int depth)
{
    std::lock_guard<std::mutex> lock_mtx(mtx);
    hasPendingChanges=false;
    if(rootFile)
    {
        FileEntryView::ViewOptions options;
        int64_t fullSpace=0;
        int64_t unknownSpace=totalSpace-rootFile->get_size()-freeSpace;

        if((flags & FileEntryView::INCLUDE_AVAILABLE_SPACE) != 0)
        {
            options.freeSpace = freeSpace;
            fullSpace+=freeSpace;
        }
        if((flags & FileEntryView::INCLUDE_UNKNOWN_SPACE) != 0)
        {
            options.unknownSpace = unknownSpace;
            fullSpace+=unknownSpace;
        }

        auto file = rootFile->findEntry(filepath);
        if(file)
        {
            fullSpace=0;
            //we don't show unknown and free space if child is opened
            options.freeSpace = 0;
            options.unknownSpace = 0;
        }
        else
            file = rootFile.get();

        fullSpace+=file->get_size();

        options.minSize = int64_t(float(fullSpace)*minSizeRatio);
        options.nestLevel = depth;
        if(view)
            FileEntryView::updateView(view, *file, options);
        else
            view= FileEntryView::createView(*file, options);
    }
}

float SpaceScanner::get_scan_progress()
{
    std::lock_guard<std::mutex> lock_mtx(mtx);
    if(scannerStatus==ScannerStatus::SCANNING && (totalSpace-freeSpace)>0)
    {
        if(rootFile)
        {
            float a=float(rootFile->get_size())/float(totalSpace-freeSpace);
            return a>1.f ? 1.f : a;
        }
        return 0.f;
    }
    return 1.f;
}

void SpaceScanner::stop_scan()
{
    {
        std::lock_guard<std::mutex> lock_mtx(mtx);
        if(scannerStatus!=ScannerStatus::STOPPING && scannerStatus!=ScannerStatus::IDLE)
            scannerStatus=ScannerStatus::STOPPING;
        scanQueue.clear();
    }
    //wait until everything is stopped
    while(scannerStatus!=ScannerStatus::IDLE)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void SpaceScanner::update_disk_space()
{
    // Should be called with locked mutex
    if(!rootFile)
        return;

    PlatformUtils::get_mount_space(rootFile->get_name(), totalSpace, freeSpace);
}

bool SpaceScanner::create_root_entry(const std::string& path)
{
    // Should be called with locked mutex
    if(rootFile)
    {
        std::cerr << "rootFile should be destroyed (cached) before creating new one\n";
        return false;
    }

    if(PlatformUtils::can_scan_dir(path))
    {
        rootFile=entryPool->create_entry(path, true);
        rootAvailable = true;
        fileCount=1;
        return true;
    }

    return true;
}

ScannerError SpaceScanner::scan_dir(const std::string &path)
{
    //do nothing if scan is already in progress
    if(scannerStatus!=ScannerStatus::IDLE)
        return ScannerError::SCAN_RUNNING;

    //we can't have multiple roots (so reset db)
    reset_database();

    auto newRootPath = Utils::make_unique<FilePath>(path);

    std::lock_guard<std::mutex> lock_mtx(mtx);
    if(!create_root_entry(newRootPath->getRoot()))
    {
        scannerStatus=ScannerStatus::IDLE;
        std::cout<<"Can't open "<<newRootPath->getRoot()<<"\n";
        return ScannerError::CANT_OPEN_DIR;
    }
    scannerStatus=ScannerStatus::SCANNING;

    rootPath = std::move(newRootPath);
    
    update_disk_space();//this will load all info about disk space (available, used, total)

    scanQueue.push_back(rootFile.get());
    hasPendingChanges = true;

    return ScannerError::NONE;
}

void SpaceScanner::rescan_dir(const FilePath& path)
{
    std::lock_guard<std::mutex> lock_mtx(mtx);
    auto entry = rootFile->findEntry(&path);
    
    if(!entry)
        return;

    scannerStatus=ScannerStatus::SCANNING;

    update_disk_space();//disk space might change since last update, so update it again

    //decrease file count by number of cached entries
    fileCount -= entry->clear_entry(entryPool.get());
    scannedSpace=rootFile->get_size();
    scanQueue.push_back(entry);
    hasPendingChanges = true;
}

uint64_t SpaceScanner::get_total_space()
{
    return totalSpace;
}

uint64_t SpaceScanner::get_scanned_space()
{
    if(!rootAvailable)
        return 0;

    auto scanned = scannedSpace.load();
    if(scanned+freeSpace>totalSpace)
        scanned=totalSpace-freeSpace;
    return scanned;
}

uint64_t SpaceScanner::get_free_space()
{
    return freeSpace;
}

const FilePath* SpaceScanner::getRootPath() const
{
    return rootPath.get();
}
