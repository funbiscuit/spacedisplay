
#ifdef _WIN32
// windows code goes here
#include "spacescanner.h"
#include "fileentry.h"
#include "utils.h"

#include <fstream>
#include <windows.h>
#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include <winbase.h>

enum FileAction {
    ADDED=1,
    REMOVED=-1,
    MODIFIED=3,
    OLD_NAME=-6,
    NEW_NAME=6
};

struct FileInfo {
    FileAction Action;
    std::string FileName;
};

void SpaceScanner::on_before_new_scan()
{

}

void SpaceScanner::watch_file_changes()
{
    //TODO
}

void ReadDirChanges(const std::string &path)
{
    auto wname=str2wstr(path);
    auto handle=CreateFileW(
            wname.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE| FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, //for async use
            nullptr
    );

    std::cout<<"Start monitoring changes\n";
    int bufSize=1024*32;
    auto buffer=new DWORD[bufSize];
    while(true)
    {
        DWORD bytesReturned=0;

        auto found=ReadDirectoryChangesW(
                handle,
                buffer,
                bufSize,
                true,
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE,
                &bytesReturned,
                nullptr,
                nullptr
        );
        int m=bytesReturned % sizeof(DWORD);
        if(m>0)
            bytesReturned+=m;


        int dwords=bytesReturned/sizeof(DWORD);

        if(found && dwords>3)
        {
            DWORD next=buffer[0]/sizeof(DWORD);//offset to next
//            std::cout<<"Bytes returned watch: "<<bytesReturned<<"\n";
            int current=0;//position to dword (NOT BYTE!)

            while(next>=0)
            {
                FileInfo fileInfo={};
                next=buffer[current]/sizeof(DWORD);//offset to next this should be dword aligned

                switch (buffer[current+1])
                {
                    case FILE_ACTION_ADDED:
                        fileInfo.Action=ADDED;
                        break;
                    case FILE_ACTION_REMOVED:
                        fileInfo.Action=REMOVED;
                        break;
                    case FILE_ACTION_MODIFIED:
                        fileInfo.Action=MODIFIED;
                        break;
                    case FILE_ACTION_RENAMED_OLD_NAME:
                        fileInfo.Action=OLD_NAME;
                        break;
                    case FILE_ACTION_RENAMED_NEW_NAME:
                        fileInfo.Action=NEW_NAME;
                        break;
                    default:
                        continue;//other reasons we are not interested in
                }

                auto nameLen=buffer[current+2]/2;
                auto name=new wchar_t[nameLen+1];

//                std::cout<<"dwords-next-current-3: "<<(dwords-next-current-3)<<", len: "<<(nameLen)<<"\n";

                memcpy(name, buffer+current+3, (dwords-next-current-3)*sizeof(DWORD)); // int is a POD
                name[nameLen]='\0';
                std::wcout<<"File name: "<<name<<", reason: "<<fileInfo.Action<<"\n";
                auto cname=wstr2str(name);
                fileInfo.FileName=cname;

                delete[](name);

                current+=next;

                if(next==0)
                    break;

            }

//            for(int k=0;k<dwords;k++)
//            {
//                printf_s("DWORD %d: %06X\n",k,buffer[k]);
//
//            }

        }

        //TODO will not happen
        if(m>10)
            break;

    }
    delete[](buffer);
    std::cout<<"Stop monitoring changes\n";

    CloseHandle(handle);
}

void SpaceScanner::init_platform()
{

}

void SpaceScanner::cleanup_platform()
{
}

void SpaceScanner::read_available_drives()
{
    auto drivesMask=GetLogicalDrives();

    char driveName[4];
    driveName[1]=':';
    driveName[2]='\\';
    driveName[3]='\0';

    availableRoots.clear();

    for(char drive='A';drive<='Z';drive++)
    {
        bool isSet=(drivesMask & 0x1)!=0;

        if(isSet)
        {
            driveName[0]=drive;

            auto type=GetDriveTypeA(driveName);

            if(type==DRIVE_FIXED || type==DRIVE_REMOVABLE)
                availableRoots.emplace_back(driveName);

//            std::cout<<"Drive "<<drive<<" is available with type "<<type<<"\n";

        }
//        else
//            std::cout<<"Drive "<<drive<<" is not available\n";
        drivesMask>>=1;
    }

}

void SpaceScanner::check_disk_space()
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

FileEntry* SpaceScanner::create_root_entry(const char* path)
{
    //we can't have multiple roots (so empty array)
    reset_database();

    auto cname=new char[strlen(path)+1];
    strcpy(cname, path);

    auto wname=str2wstr(cname);

    auto handle=CreateFileW(
            wname.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE| FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            nullptr
    );


    auto parent=entryPool.create_entry(fileCount,cname, FileEntry::DIRECTORY);
    delete[](cname);

    if(handle!=INVALID_HANDLE_VALUE)
    {
        BY_HANDLE_FILE_INFORMATION lpFileInformation;
        if(GetFileInformationByHandle(handle, &lpFileInformation))
        {
            if(!(lpFileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                CloseHandle(handle);
                return nullptr;
            }

            uint64_t size=(uint64_t(lpFileInformation.nFileSizeHigh) * (uint64_t(MAXDWORD)+1)) + uint64_t(lpFileInformation.nFileSizeLow);

            parent->set_size(size);
            //mtx.lock();
            rootFile=parent;
            ++fileCount;
            //mtx.unlock();
        } else
        {
            CloseHandle(handle);
            return nullptr;
        }
    }
    else
        return nullptr;

    CloseHandle(handle);

    return parent;
}


void SpaceScanner::scan_dir_prv(FileEntry *parent)
{
    if(!parent || !parent->is_dir())
        return;


    std::string path;
    parent->get_path(path);
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

    //std::cout<<"Cap: "<<files.capacity()<<", size: "<<files.size()<<", dif:"<<(files.capacity()-files.size())<<"\n";

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
            //size+=size % 4096;

            auto fe=entryPool.create_entry(fileCount,cname.c_str(), isDir ? FileEntry::DIRECTORY : FileEntry::FILE);
            fe->set_size(size);

            mtx.lock();
            fe->set_parent(parent);
            //parent->add_child(fe);
            ++fileCount;
            mtx.unlock();

            if(isDir)
                scan_dir_prv(fe);
        }
        found=(scannerStatus==SCANNING) && FindNextFileW(handle, &fileData)!=0;
    }
    FindClose(handle);
    hasPendingChanges = true;
}

#endif
