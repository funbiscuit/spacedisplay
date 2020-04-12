
#include "spacescanner.h"
#include "fileentry.h"
#include "fileentryshared.h"
#include "fileentrypool.h"
#include "platformutils.h"

#include <fstream>
#include <iostream>
#include <cstring>
#include <chrono>

SpaceScanner::SpaceScanner() :
        scannerStatus(ScannerStatus::IDLE), runWorker(false)
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
        using namespace std::chrono;
        //start scanning as soon as something is placed in queue
        if(!scanQueue.empty())
        {
            auto start = high_resolution_clock::now();
            while(!scanQueue.empty() && runWorker)
            {
                FileEntry* entry = nullptr;
                {
                    std::lock_guard<std::mutex> guard(mtx);
                    if(!scanQueue.empty())
                    {
                        entry = scanQueue.front();
                        scanQueue.pop_front();
                    }
                }
                if(entry)
                    update_entry_children(entry);

                if(scannerStatus==ScannerStatus::STOPPING)
                    scanQueue.clear();
            }

            auto stop   = high_resolution_clock::now();
            auto mseconds = duration_cast<milliseconds>(stop - start).count();
            std::cout << "Time taken: " << mseconds <<"ms, "<<fileCount<<" file(s) scanned\n";
            scannerStatus = ScannerStatus::IDLE;
        }
        std::this_thread::sleep_for(milliseconds(20));
    }

    std::cout << "End worker thread\n";
}

void SpaceScanner::update_entry_children(FileEntry* entry)
{
    if(!entry->is_dir())
        return;

    std::string path;
    entry->get_path(path);

    //TODO add check if iterator was constructed and we were able to open path
    for(FileIterator it(path); it.is_valid(); ++it)
    {
        auto fe=entryPool->create_entry(fileCount, it.name, it.isDir ? FileEntry::DIRECTORY : FileEntry::FILE);
        fe->set_size(it.size);

        std::lock_guard<std::mutex> lock_mtx(mtx);
        auto fe_raw = fe.get();
        entry->add_child(std::move(fe));
        ++fileCount;

        // this section is important for linux since not any path should be scanned (e.g. /proc or /sys)
        if(it.isDir)
        {
            std::string newPath;
            fe_raw->get_path(newPath);
            if(newPath.back() != '/')
                newPath.append("/");
            if(!Utils::in_array(newPath, availableRoots) && !Utils::in_array(newPath, excludedMounts))
                scanQueue.push_front(fe_raw);
            else
                std::cout<<"Skip scan of: "<<newPath<<"\n";
        }
    }

    hasPendingChanges = true;
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
    entryPool->cache_children(std::move(rootFile));
}

bool SpaceScanner::can_refresh()
{
    return rootFile!= nullptr && !is_running();
}

bool SpaceScanner::is_running()
{
    return scannerStatus!=ScannerStatus::IDLE;
}

bool SpaceScanner::is_loaded()
{
    return rootFile!= nullptr;
}

FileEntrySharedPtr SpaceScanner::get_root_file(float minSizeRatio, uint16_t flags, const char* filepath, int depth)
{
    std::lock_guard<std::mutex> lock_mtx(mtx);
    
    if(rootFile!= nullptr)
    {
        FileEntryShared::CopyOptions options;
        int64_t fullSpace=0;
        int64_t unknownSpace=totalSpace-rootFile->get_size()-freeSpace;

        if((flags & FileEntryShared::INCLUDE_AVAILABLE_SPACE)!=0)
        {
            options.freeSpace = freeSpace;
            fullSpace+=freeSpace;
        }
        if((flags & FileEntryShared::INCLUDE_UNKNOWN_SPACE)!=0)
        {
            options.unknownSpace = unknownSpace;
            fullSpace+=unknownSpace;
        }

        auto file = rootFile.get();

        if(strlen(filepath)>0)
        {
            auto rootLen = strlen(rootFile->get_name());
            if(strncmp(rootFile->get_name(), filepath, rootLen)==0)
            {
                filepath = filepath + rootLen*sizeof(char);
            }

            auto test = file->find_child_dir(filepath);
            if(test!= nullptr)
            {
                file=test;
                fullSpace=0;
                //we don't show unknown and free space if child is opened
                options.freeSpace = 0;
                options.unknownSpace = 0;
            }
        }
        fullSpace+=file->get_size();

        options.minSize = int64_t(float(fullSpace)*minSizeRatio);
        options.nestLevel = depth;
        auto sharedCopy=FileEntryShared::create_copy(*file, options);

        return sharedCopy;
    }
    return nullptr;
}

bool SpaceScanner::has_changes()
{
    return hasPendingChanges;
}

void SpaceScanner::update_root_file(FileEntrySharedPtr& root, float minSizeRatio, uint16_t flags, const char* filepath, int depth)
{
    std::lock_guard<std::mutex> lock_mtx(mtx);
    hasPendingChanges=false;
    if(rootFile!= nullptr)
    {
        FileEntryShared::CopyOptions options;
        int64_t fullSpace=0;
        int64_t unknownSpace=totalSpace-rootFile->get_size()-freeSpace;

        if((flags & FileEntryShared::INCLUDE_AVAILABLE_SPACE)!=0)
        {
            options.freeSpace = freeSpace;
            fullSpace+=freeSpace;
        }
        if((flags & FileEntryShared::INCLUDE_UNKNOWN_SPACE)!=0)
        {
            options.unknownSpace = unknownSpace;
            fullSpace+=unknownSpace;
        }

        auto file = rootFile.get();

        if(strlen(filepath)>0)
        {
            auto rootLen = strlen(rootFile->get_name());
            if(strncmp(rootFile->get_name(), filepath, rootLen)==0)
            {
                filepath = filepath + rootLen*sizeof(char);
            }

            auto test = file->find_child_dir(filepath);
            if(test!= nullptr)
            {
                file=test;
                fullSpace=0;
                //we don't show unknown and free space if child is opened
                options.freeSpace = 0;
                options.unknownSpace = 0;
            }
        }
        fullSpace+=file->get_size();

        options.minSize = int64_t(float(fullSpace)*minSizeRatio);
        options.nestLevel = depth;
        FileEntryShared::update_copy(root, *file, options);
    }
}

float SpaceScanner::get_scan_progress()
{
    std::lock_guard<std::mutex> lock_mtx(mtx);
    if(scannerStatus==ScannerStatus::SCANNING && (totalSpace-freeSpace)>0)
    {
        if(rootFile!=nullptr)
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
    if(!rootFile)
        return;

    PlatformUtils::get_mount_space(rootFile->get_name(), totalSpace, freeSpace);

    std::cout<<"Total: "<<totalSpace<<", free: "<<freeSpace<<"\n";
}

bool SpaceScanner::create_root_entry(const std::string& path)
{
    if(rootFile)
    {
        std::cerr << "rootFile should be destroyed (cached) before creating new one\n";
        return false;
    }

    if(PlatformUtils::can_scan_dir(path))
    {
        rootFile=entryPool->create_entry(fileCount, path, FileEntry::DIRECTORY);
        ++fileCount;
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

    scannerStatus=ScannerStatus::SCANNING;

    if(!create_root_entry(path))
    {
        scannerStatus=ScannerStatus::IDLE;
        std::cout<<"Can't open "<<path.c_str()<<"\n";
        return ScannerError::CANT_OPEN_DIR;
    }
    
    update_disk_space();//this will load all info about disk space (available, used, total)

    std::lock_guard<std::mutex> lock_mtx(mtx);
    scanQueue.push_back(rootFile.get());
    hasPendingChanges = true;

    return ScannerError::NONE;
}

void SpaceScanner::rescan_dir(const std::string &path)
{
    auto entry = getEntryAt(path.c_str());
    
    if(!entry)
        return;

    scannerStatus=ScannerStatus::SCANNING;

    std::lock_guard<std::mutex> lock_mtx(mtx);

    update_disk_space();//disk space might change since last update, so update it again

    entry->clear_entry(entryPool.get());
    scanQueue.push_back(entry);
    hasPendingChanges = true;
}

FileEntry* SpaceScanner::getEntryAt(const char* path)
{
    std::lock_guard<std::mutex> lock_mtx(mtx);
    if(!rootFile)
        return nullptr;

    auto rootLen = strlen(rootFile->get_name());
    if(strncmp(rootFile->get_name(), path, rootLen)==0)
    {
        path = &path[rootLen];
    }
    auto file = rootFile.get();

    file = file->find_child_dir(path);
    if(!file)
        file=rootFile.get();

    return file;
}

uint64_t SpaceScanner::get_total_space()
{
    return totalSpace;
}

uint64_t SpaceScanner::get_scanned_space()
{
    if(!rootFile)
        return 0;

    auto scanned = rootFile->get_size();
    if(scanned+freeSpace>totalSpace)
        scanned=totalSpace-freeSpace;
    return scanned;
}

uint64_t SpaceScanner::get_free_space()
{
    return freeSpace;
}

const char *SpaceScanner::get_root_path()
{
    if(rootFile)
        return rootFile->get_name();
    return "";
}
