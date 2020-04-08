
#include "spacescanner.h"
#include "fileentry.h"
#include "fileentryshared.h"
#include "fileentrypool.h"

#include <fstream>
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>

SpaceScanner::SpaceScanner() :
        scannerStatus(ScannerStatus::IDLE)
{
    entryPool = Utils::make_unique<FileEntryPool>();
    init_platform();
    //Start thread after everything is initialized
    workerThread=std::thread(&SpaceScanner::worker_run, this);
}

SpaceScanner::~SpaceScanner()
{
    cleanup_platform();
    runWorker = false;
    workerThread.join();
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

std::vector<std::string> SpaceScanner::get_available_roots()
{
    read_available_drives();

    return availableRoots;
}

void SpaceScanner::reset_database()
{
    stop_scan();
    std::lock_guard<std::mutex> lock_mtx(mtx);
    fileCount=0;
    entryPool->cache_children(rootFile);
    rootFile= nullptr;
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
        int64_t fullSpace=0;
        int64_t unknownSpace=totalSpace-rootFile->get_size()-freeSpace;

        if((flags & FileEntryShared::INCLUDE_AVAILABLE_SPACE)!=0)
            fullSpace+=freeSpace;
        if((flags & FileEntryShared::INCLUDE_UNKNOWN_SPACE)!=0)
            fullSpace+=unknownSpace;

        auto file = rootFile;

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
            }
        }
        fullSpace+=file->get_size();
        
        auto minSize=int64_t(float(fullSpace)*minSizeRatio);
        auto sharedCopy=FileEntryShared::create_copy(*file, depth, minSize, flags);

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
        int64_t fullSpace=0;
        int64_t unknownSpace=totalSpace-rootFile->get_size()-freeSpace;

        if((flags & FileEntryShared::INCLUDE_AVAILABLE_SPACE)!=0)
            fullSpace+=freeSpace;
        if((flags & FileEntryShared::INCLUDE_UNKNOWN_SPACE)!=0)
            fullSpace+=unknownSpace;

        auto file = rootFile;

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
            }
        }
        fullSpace+=file->get_size();

        auto minSize=int64_t(float(fullSpace)*minSizeRatio);
        FileEntryShared::update_copy(root, *file, depth, minSize, flags);
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
    //wait until everythin is stopped
    while(scannerStatus!=ScannerStatus::IDLE)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

ScannerError SpaceScanner::scan_dir(const std::string &path)
{
    //do nothing if scan is already in progress
    if(scannerStatus!=ScannerStatus::IDLE)
        return ScannerError::SCAN_RUNNING;

    //we can't have multiple roots (so reset db)
    reset_database();

    scannerStatus=ScannerStatus::SCANNING;

    auto parent=create_root_entry(path.c_str());
    if(!parent)
    {
        scannerStatus=ScannerStatus::IDLE;
        std::cout<<"Can't open "<<path.c_str()<<"\n";
        return ScannerError::CANT_OPEN_DIR;
    }
    
    check_disk_space();//this will load all info about disk space (available, used, total)
    
    //create unknown space and free space children
    auto unknownSpace=totalSpace-freeSpace;
    unknownSpace = unknownSpace<0 ? 0 : unknownSpace;
    
    
    auto fe=entryPool->create_entry(fileCount,"", FileEntry::AVAILABLE_SPACE);
    fe->set_size(freeSpace);
    fe->set_parent(parent);
    ++fileCount;
    
    fe=entryPool->create_entry(fileCount,"", FileEntry::UNKNOWN_SPACE);
    fe->set_size(unknownSpace);
    fe->set_parent(parent);
    ++fileCount;

    std::lock_guard<std::mutex> lock_mtx(mtx);
    scanQueue.push_back(rootFile);
    hasPendingChanges = true;

    return ScannerError::NONE;
}

void SpaceScanner::rescan_dir(const std::string &path)
{
    auto entry = getEntryAt(path.c_str());
    
    if(!entry)
        return;

    std::lock_guard<std::mutex> lock_mtx(mtx);

    check_disk_space();//disk space might change since last update, so update it again
    rootFile->update_free_space(freeSpace);

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
    auto file = rootFile;

    file = file->find_child_dir(path);
    if(!file)
        file=rootFile;

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
