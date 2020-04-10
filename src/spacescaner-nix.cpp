
#ifdef __linux__
#include "spacescanner.h"
#include "fileentry.h"
#include "fileentrypool.h"
#include "utils.h"

#include <regex>
#include <dirent.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <chrono>
#include <sys/statvfs.h>
#include <unordered_map>
#include <thread>

namespace SpaceScannerPrv
{
    std::vector<std::string> partitions;
}


void SpaceScanner::init_platform()
{
    using namespace SpaceScannerPrv;
    partitions.clear();
    //TODO add missing partitions
    partitions.emplace_back("ext2");
    partitions.emplace_back("ext3");
    partitions.emplace_back("ext4");
    partitions.emplace_back("vfat");
    partitions.emplace_back("ntfs");
    partitions.emplace_back("fuseblk");
}

void SpaceScanner::cleanup_platform()
{
}

void SpaceScanner::update_available_drives()
{
    excludedMounts.clear();
    availableRoots.clear();
    
    std::ifstream file("/proc/mounts");
    std::string str;
    while (std::getline(file, str))
    {
        size_t pos=0;
        int counter=0;
        size_t typeStart=0;
        size_t typeEnd=0;
        size_t mntStart=0;
        size_t mntEnd=0;
        
        //we need to divide string by spaces to get info about partition type and mount point
        //structure is as follows:
        //device mount-point partition-type other-stuff
        while((pos=str.find(' ',pos+1))!=std::string::npos)
        {
            counter++;
            if(counter==1)
                mntStart=pos+1;
            else if(counter==2)
            {
                typeStart=pos+1;
                mntEnd=pos;
            }
            else if(counter==3)
                typeEnd=pos;
        }
        if((typeStart>0 && typeEnd>0 && typeEnd>typeStart) && (mntStart>0 && mntEnd>0 && mntEnd>mntStart))
        {
            auto part=str.substr(typeStart,typeEnd-typeStart);
            auto mount=str.substr(mntStart,mntEnd-mntStart);
            mount = regex_replace(mount, std::regex("\\\\040"), " ");

            if(mount[mount.length()-1] != '/')
                mount.append("/");
            if(in_array(part, SpaceScannerPrv::partitions))
            {
                availableRoots.push_back(mount);
            } else{
                excludedMounts.push_back(mount);
            }
        }
    }
}

void SpaceScanner::update_disk_space()
{
    if(!rootFile)
        return;

    struct statvfs disk_stat;
    auto res= statvfs(rootFile->get_name(), &disk_stat);

    totalSpace=uint64_t(disk_stat.f_frsize)*uint64_t(disk_stat.f_blocks);
    freeSpace=uint64_t(disk_stat.f_bavail)*uint64_t(disk_stat.f_frsize);

    std::cout<<"Total: "<<totalSpace<<", free: "<<freeSpace<<"\n";
}

bool SpaceScanner::create_root_entry(const std::string& path)
{
    if(rootFile)
    {
        std::cerr << "rootFile should be destroyed (cached) before creating new one\n";
        return false;
    }

    struct stat file_stat{};
    int status;

    auto parent=entryPool->create_entry(fileCount, path, FileEntry::DIRECTORY);

    status = lstat(path.c_str(), &file_stat);


    if(status==0)
    {
        bool isDir=S_ISDIR(file_stat.st_mode);
        if(!isDir)
            return false;
        parent->set_size(file_stat.st_size);
        rootFile=std::move(parent);
        fileCount++;
    }
    else
        return false;


    return true;
}

void SpaceScanner::on_before_new_scan()
{

}

void SpaceScanner::update_entry_children(FileEntry *entry)
{
    if(!entry->is_dir())
        return;
    
    DIR *dirp;
    struct dirent *dp;

    std::string path;
    entry->get_path(path);
    
    if ((dirp = opendir(path.c_str())) == nullptr) {
        std::cout << "Couldn't open "<<path<<"\n";
        return;
    }

    struct stat file_stat{};
    int status;

    while((dp = readdir(dirp)) != nullptr)
    {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            continue;
    
        auto nameLen=strlen(dp->d_name);

        std::string child = path;
        child.append("/");
        child.append( dp->d_name);

        status = lstat(child.c_str(), &file_stat);

        if(status==0)
        {
            bool isDir=S_ISDIR(file_stat.st_mode);
            
            totalSize+=nameLen+2;
    
            auto fe=entryPool->create_entry(fileCount,dp->d_name, isDir ? FileEntry::DIRECTORY : FileEntry::FILE);
            fe->set_size(file_stat.st_size);

            std::lock_guard<std::mutex> lock_mtx(mtx);
            auto fe_raw = fe.get();
            entry->add_child(std::move(fe));
            ++fileCount;
            
            if(isDir)
            {
                std::string newPath;
                fe_raw->get_path(newPath);
                if(newPath.back() != '/')
                    newPath.append("/");
                if(!in_array(newPath, availableRoots) && !in_array(newPath, excludedMounts))
                    scanQueue.push_front(fe_raw);
                else
                    std::cout<<"Skip scan of: "<<newPath<<"\n";
            }

        }
    }

    hasPendingChanges = true;
    
    closedir(dirp);
    
}

#endif

