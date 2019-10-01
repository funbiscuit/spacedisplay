
#include "spacescanner.h"
#include "fileentry.h"

#include <fstream>
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>

SpaceScanner::SpaceScanner(): scannerStatus(ScannerStatus::IDLE), rootFile(nullptr)
{
    init_platform();
}

SpaceScanner::~SpaceScanner()
{
    cleanup_platform();
}

bool SpaceScanner::on_timeout()
{
    if(hasPendingChanges)
    {
        hasPendingChanges = false;
//        m_signal_data_changed.emit();
    }

    return true;
}

std::vector<std::string> SpaceScanner::get_available_roots()
{
    read_available_drives();

    return availableRoots;
}

void SpaceScanner::reset_database()
{
    mtx.lock();
    rootFile= nullptr;
    fileCount=0;
    entryPool.destroy_entries();
    mtx.unlock();
}

void SpaceScanner::lock()
{
    mtx.lock();
}
bool SpaceScanner::try_lock()
{
    return mtx.try_lock();
}
void SpaceScanner::unlock()
{
    mtx.unlock();
}

void SpaceScanner::async_scan()
{
    std::cout<<"Start scanning\n";

    using namespace std::chrono;
    auto start   = high_resolution_clock::now();

    on_before_new_scan();
    totalSize=0;
    scan_dir_prv(rootFile);

    auto stop   = high_resolution_clock::now();

    auto mseconds = duration_cast<milliseconds>(stop - start).count();

    std::cout << "Time taken: " << mseconds<<"ms, "<<fileCount<<" file(s) scanned\n";

    scannerStatus=ScannerStatus::IDLE;
    hasPendingChanges = true;
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
    mtx.lock();
    
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

        mtx.unlock();
        return sharedCopy;
    }
    mtx.unlock();
    return nullptr;
}

bool SpaceScanner::has_changes()
{
    return hasPendingChanges;
}

void SpaceScanner::update_root_file(FileEntrySharedPtr& root, float minSizeRatio, uint16_t flags, const char* filepath, int depth)
{
    mtx.lock();
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
    mtx.unlock();
}

float SpaceScanner::get_scan_progress()
{
    mtx.lock();
    if(scannerStatus==ScannerStatus::SCANNING && (totalSpace-freeSpace)>0)
    {
        if(rootFile!=nullptr)
        {
            float a=float(rootFile->get_size())/float(totalSpace-freeSpace);
            mtx.unlock();
            return a>1.f ? 1.f : a;
        }
        mtx.unlock();
        return 0.f;
    }
    mtx.unlock();
    return 1.f;
}

void SpaceScanner::stop_scan()
{
    if(scannerStatus!=ScannerStatus::STOPPING && scannerStatus!=ScannerStatus::IDLE)
        scannerStatus=ScannerStatus::STOPPING;
}

void SpaceScanner::scan_dir(const std::string &path)
{
    //do nothing if scan is already in progress
    if(scannerStatus!=ScannerStatus::IDLE)
        return;

    scannerStatus=ScannerStatus::SCANNING;

    auto parent=create_root_entry(path.c_str());
    if(!parent)
    {
        scannerStatus=ScannerStatus::IDLE;
        std::cout<<"Can't open "<<path.c_str()<<"\n";
        return;
    }
    
    check_disk_space();//this will load all info about disk space (available, used, total)
    
    //create unknown space and free space children
    auto unknownSpace=totalSpace-freeSpace;
    unknownSpace = unknownSpace<0 ? 0 : unknownSpace;
    
    
    auto fe=entryPool.create_entry(fileCount,"", FileEntry::AVAILABLE_SPACE);
    fe->set_size(freeSpace);
    fe->set_parent(parent);
    ++fileCount;
    
    fe=entryPool.create_entry(fileCount,"", FileEntry::UNKNOWN_SPACE);
    fe->set_size(unknownSpace);
    fe->set_parent(parent);
    ++fileCount;

    std::cout<<"Start scan thread\n";
    std::thread t(&SpaceScanner::async_scan, this);
    t.detach();
    hasPendingChanges = true;

    //std::thread thr(ReadDirChanges, path);
    //thr.detach();


    //TODO
    //std::thread thr(ReadDirChanges, path);
    //thr.detach();


}

void SpaceScanner::rescan_dir(const std::string &path)
{
    //do nothing if scan is already in progress
    if(scannerStatus!=ScannerStatus::IDLE)
        return;

    rescanPathQueue.push(path);
    check_disk_space();//disk space might change since last update, so update it again
    if(rootFile)
        rootFile->update_free_space(freeSpace);

    scannerStatus=ScannerStatus::SCANNING;
    std::cout<<"Start rescan thread\n";
    std::thread t(&SpaceScanner::async_rescan, this);
    t.detach();
}


void SpaceScanner::async_rescan()
{
    if(rescanPathQueue.empty())
        return;

    std::cout<<"Start rescanning\n";

    using namespace std::chrono;
    auto start   = high_resolution_clock::now();

    bool rootRescan = false;

    while (!rescanPathQueue.empty())
    {
        auto file = rootFile;

        auto path = rescanPathQueue.front();
        rescanPathQueue.pop();

        if(path.length()>0)
        {
            auto rootLen = strlen(rootFile->get_name());
            auto filepath = path.c_str();
            if(strncmp(rootFile->get_name(), filepath, rootLen)==0)
            {
                filepath = filepath + rootLen*sizeof(char);
            }

            auto test = file->find_child_dir(filepath);
            if(test!= nullptr)
            {
                file=test;
                file->clear_entry(&entryPool);

                hasPendingChanges = true;
                scan_dir_prv(file);
            }
            else
            {
                rootRescan=true;
                //for rescanning of root just start a new scan
                scannerStatus = ScannerStatus::IDLE;
                scan_dir(path);
            }
        }
    }


    if(!rootRescan)
    {
        auto stop   = high_resolution_clock::now();

        auto mseconds = duration_cast<milliseconds>(stop - start).count();

        std::cout << "Time taken: " << mseconds<<"ms, "<<fileCount<<" file(s) scanned\n";

        scannerStatus=ScannerStatus::IDLE;
    }
    hasPendingChanges = true;
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
