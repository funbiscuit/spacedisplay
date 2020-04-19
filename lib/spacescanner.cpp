
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
        FilePath path("/");
        while(!scanQueue.empty() && runWorker && scannerStatus==ScannerStatus::SCANNING)
        {
            auto entry = scanQueue.front();
            scanQueue.pop_front();
            entry->getPath(path);
            dbLock.unlock();

            scanChildrenAt(path, scannedEntries);

            dbLock.lock();

            if(scannerStatus!=ScannerStatus::SCANNING)
                break;

            // Presorting entries by size so we can insert them much quicker
            if(scannedEntries.size()>10)
            {
                std::sort(scannedEntries.begin(), scannedEntries.end(), [](ScannedEntry& e1, ScannedEntry& e2)
                {
                    return e1.entry->get_size() > e2.entry->get_size();
                });
            }

            //if between two locks our structure changed, our previous pointer might be invalid
            //so we need to find again entry, for which children were scanned
            entry = rootFile->findEntry(&path);
            if(entry)
            {
                while(!scannedEntries.empty())
                {
                    auto p = std::move(scannedEntries.back());
                    scannedEntries.pop_back();

                    if(p.addToQueue)
                        scanQueue.push_front(p.entry.get());
                    entry->add_child(std::move(p.entry));
                    ++fileCount;
                    hasPendingChanges = true;
                }
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

void SpaceScanner::scanChildrenAt(const FilePath& path, std::vector<ScannedEntry>& scannedEntries)
{
    auto pathStr = path.getPath();

    //TODO add check if iterator was constructed and we were able to open path
    for(FileIterator it(pathStr); it.is_valid(); ++it)
    {
        auto fe=entryPool->create_entry(it.name, it.isDir);
        fe->set_size(it.size);

        bool addToQueue = it.isDir;
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
        scannedEntries.push_back({std::move(fe), addToQueue});
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
        if(file != rootFile.get())
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

    return false;
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
