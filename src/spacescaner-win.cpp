
#ifdef _WIN32
// windows code goes here
#include "spacescanner.h"
#include "fileentry.h"
#include "fileentrypool.h"
#include "utils.h"

#include <fstream>
#include <windows.h>
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include <winbase.h>

void SpaceScanner::on_before_new_scan()
{

}

void SpaceScanner::init_platform()
{

}

void SpaceScanner::cleanup_platform()
{
}

void SpaceScanner::update_available_drives()
{
    auto drivesMask=GetLogicalDrives();

    char driveName[4];
    driveName[1]=':';
    driveName[2]='\\';
    driveName[3]='\0';

    availableRoots.clear();

    for(char drive='A';drive<='Z';drive++)
    {
        if(drivesMask & 0x1)
        {
            driveName[0]=drive;

            auto type=GetDriveTypeA(driveName);

            if(type==DRIVE_FIXED || type==DRIVE_REMOVABLE)
                availableRoots.emplace_back(driveName);
        }
        drivesMask>>=1;
    }
}

void SpaceScanner::update_disk_space()
{
    if(!rootFile)
        return;
    
    auto wname=str2wstr(rootFile->get_name());

    ULARGE_INTEGER totalNumberOfBytes;
    ULARGE_INTEGER totalNumberOfFreeBytes;
    ULARGE_INTEGER freeBytesAvailableToCaller;

    GetDiskFreeSpaceExW(wname.c_str(),&freeBytesAvailableToCaller,&totalNumberOfBytes,&totalNumberOfFreeBytes);
    totalSpace=totalNumberOfBytes.QuadPart;
    freeSpace=freeBytesAvailableToCaller.QuadPart;

    std::cout<<"Total: "<<totalSpace<<", free: "<<freeSpace<<"\n";
}

bool SpaceScanner::create_root_entry(const std::string& path)
{
    if(rootFile)
    {
        std::cerr << "rootFile should be destroyed (cached) before creating new one\n";
        return false;
    }

    auto wname=str2wstr(path);

    auto handle=CreateFileW(
            wname.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE| FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            nullptr
    );


    auto parent=entryPool->create_entry(fileCount, path, FileEntry::DIRECTORY);

    if(handle!=INVALID_HANDLE_VALUE)
    {
        BY_HANDLE_FILE_INFORMATION lpFileInformation;
        if(GetFileInformationByHandle(handle, &lpFileInformation))
        {
            if(!(lpFileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                CloseHandle(handle);
                return false;
            }

            uint64_t size=(uint64_t(lpFileInformation.nFileSizeHigh) * (uint64_t(MAXDWORD)+1)) + uint64_t(lpFileInformation.nFileSizeLow);

            parent->set_size(size);
            //mtx.lock();
            rootFile=std::move(parent);
            ++fileCount;
            //mtx.unlock();
        } else
        {
            CloseHandle(handle);
            return false;
        }
    }
    else
        return false;

    CloseHandle(handle);
    return true;
}

void SpaceScanner::update_entry_children(FileEntry* entry)
{
    if(!entry->is_dir())
        return;


    std::string path;
    entry->get_path(path);
    path.append("\\*");

    auto wname=str2wstr(path);

    static WIN32_FIND_DATAW fileData;

    auto handle=FindFirstFileW(wname.c_str(), &fileData);

    bool found=handle!=INVALID_HANDLE_VALUE;

    if(!found)
    {
        //std::wcout << "Couldn't open "<<wname<<"\n";
        return;
    }

    while(found)
    {
        auto cname=wstr2str(fileData.cFileName);

        if (cname != "." && cname != "..")
        {
            bool isDir=(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)!=0;
            bool hasReparse=(fileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)!=0;

            if(hasReparse)
            {
                //don't scan mount points and symlinks since they should not account in total space
                //auto wpath=utf8_to_wchar(parent->get_path());
                if(fileData.dwReserved0==IO_REPARSE_TAG_SYMLINK)
                {
//                    std::wcout<<"Symlink:"<<wpath<<"/"<<fileData.cFileName<<"\n";
                    isDir=false;
                }
                else if(fileData.dwReserved0==IO_REPARSE_TAG_MOUNT_POINT)
                {
//                    std::wcout<<"Mount point:"<<wpath<<"/"<<fileData.cFileName<<"\n";
                    isDir=false;
                }
//                else
//                    std::wcout<<"Other reparse:"<<wpath<<"/"<<fileData.cFileName<<", "<<fileData.dwReserved0<<"\n";
            }

            uint64_t size=(uint64_t(fileData.nFileSizeHigh) * (uint64_t(MAXDWORD)+1)) + uint64_t(fileData.nFileSizeLow);

            auto fe=entryPool->create_entry(fileCount,cname, isDir ? FileEntry::DIRECTORY : FileEntry::FILE);
            fe->set_size(size);

            std::lock_guard<std::mutex> lock_mtx(mtx);
            auto fe_raw = fe.get();
            entry->add_child(std::move(fe));
            ++fileCount;
            if(isDir)
                scanQueue.push_front(fe_raw);
        }
        found = FindNextFileW(handle, &fileData) != 0;
    }
    FindClose(handle);
    hasPendingChanges = true;
}

#endif
