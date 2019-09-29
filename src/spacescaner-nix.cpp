
#ifdef __linux__
#include "spacescanner.h"
#include "fileentry.h"
#include "utils.h"

#include <sys/inotify.h>
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
    int inotifyFd;
    std::vector<std::string> partitions;

    int watchedDirs = 0;
    std::unordered_map<int, FileEntry*> inotifyWdMap;
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

    inotifyFd = inotify_init1(IN_NONBLOCK);
    if (inotifyFd == -1)
    {
        std::cerr << "Failed to initialize inotify!\n";
        return;
    }
}

void SpaceScanner::cleanup_platform()
{
}


/* Display information from inotify_event structure */
static void displayInotifyEvent(struct inotify_event *i)
{
    if (i->mask & IN_IGNORED)
        return;

    using namespace SpaceScannerPrv;

    auto search = inotifyWdMap.find(i->wd);
    std::string path;
    if(search != inotifyWdMap.end())
    {
        auto entry = (FileEntry*)search->second;
        entry->get_path(path);
        printf("    path =%s; ", path.c_str());
    }
    else
        printf("    wd =%2d; ", i->wd);
    if (i->cookie > 0)
        printf("cookie =%4d; ", i->cookie);

    printf("mask = ");
    if (i->mask & IN_ACCESS)        printf("IN_ACCESS ");
    if (i->mask & IN_ATTRIB)        printf("IN_ATTRIB ");
    if (i->mask & IN_CLOSE_NOWRITE) printf("IN_CLOSE_NOWRITE ");
    if (i->mask & IN_CLOSE_WRITE)   printf("IN_CLOSE_WRITE ");
    if (i->mask & IN_CREATE)        printf("IN_CREATE ");
    if (i->mask & IN_DELETE)        printf("IN_DELETE ");
    if (i->mask & IN_DELETE_SELF)   printf("IN_DELETE_SELF ");
    if (i->mask & IN_ISDIR)         printf("IN_ISDIR ");
    if (i->mask & IN_MODIFY)        printf("IN_MODIFY ");
    if (i->mask & IN_MOVE_SELF)     printf("IN_MOVE_SELF ");
    if (i->mask & IN_MOVED_FROM)    printf("IN_MOVED_FROM ");
    if (i->mask & IN_MOVED_TO)      printf("IN_MOVED_TO ");
    if (i->mask & IN_OPEN)          printf("IN_OPEN ");
    if (i->mask & IN_Q_OVERFLOW)    printf("IN_Q_OVERFLOW ");
    if (i->mask & IN_UNMOUNT)       printf("IN_UNMOUNT ");
    printf("\n");

    if (i->len > 0)
        printf("        name = %s\n", i->name);
}


int inotify_watch_dir(const char* dir_path)
{
    int wd;

    const auto EVENTS = (IN_MODIFY | IN_MOVED_FROM	| IN_MOVED_TO \
                        | IN_CREATE | IN_DELETE	| IN_DELETE_SELF | IN_MOVE_SELF);

    wd = inotify_add_watch(SpaceScannerPrv::inotifyFd, dir_path, EVENTS);
    if (wd == -1)
    {
        std::cerr << "Failed to add inotify watch to " << dir_path<< "\n";
        return -1;
    }

    return wd;
}


void inotify_check_events()
{
    const size_t BUF_LEN = (10 * (sizeof(struct inotify_event) + NAME_MAX + 1));

    char buf[BUF_LEN] __attribute__ ((aligned(8)));
    ssize_t numRead;
    char *p;
    struct inotify_event *event;

    for (;;)
    {
        numRead = read(SpaceScannerPrv::inotifyFd, buf, BUF_LEN);
        if (numRead <= 0)
        {
            break;
        }

        /* Process all of the events in buffer returned by read() */
        for (p = buf; p < buf + numRead; ) {
            event = (struct inotify_event *) p;
            displayInotifyEvent(event);

            p += sizeof(struct inotify_event) + event->len;
        }
    }
}


void SpaceScanner::read_available_drives()
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

void SpaceScanner::check_disk_space()
{
    if(!rootFile)
        return;
    
    
    struct statvfs disk_stat;
    
    auto res= statvfs(rootFile->get_name(), &disk_stat);
    
    
    
    totalSpace=uint64_t(disk_stat.f_frsize)*uint64_t(disk_stat.f_blocks);
    freeSpace=uint64_t(disk_stat.f_bavail)*uint64_t(disk_stat.f_frsize);
    
    //freeSpace=freeBytesAvailableToCaller.QuadPart;
    
    std::cout<<"Total: "<<totalSpace<<", free: "<<freeSpace<<"\n";
}

FileEntry* SpaceScanner::create_root_entry(const char* path)
{
    //we can't have multiple roots (so empty array)
    reset_database();


    struct stat file_stat{};
    int status;

    auto cname=new char[strlen(path)+1];
    strcpy(cname, path);
    auto parent=entryPool.create_entry(fileCount,cname, FileEntry::DIRECTORY);
    delete[](cname);

    status = lstat(path, &file_stat);


    if(status==0)
    {
        bool isDir=S_ISDIR(file_stat.st_mode);
        if(!isDir)
            return nullptr;
        parent->set_size(file_stat.st_size);
        rootFile=parent;
        fileCount++;
    }
    else
        return nullptr;


    return parent;
}

void SpaceScanner::on_before_new_scan()
{
    using namespace SpaceScannerPrv;
    for(auto it = inotifyWdMap.begin(); it != inotifyWdMap.end(); ++it)
        inotify_rm_watch(inotifyFd, it->first);

    watchedDirs = 0;
    inotifyWdMap.clear();
}

void SpaceScanner::watch_file_changes()
{
    while(true)
    {
        inotify_check_events();

        if(scannerStatus==STOPPING)
            break;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void SpaceScanner::scan_dir_prv(FileEntry *parent)
{
    if(!parent || !parent->is_dir())
        return;
    
    DIR *dirp;
    struct dirent *dp;

    std::string path;
    parent->get_path(path);
    
    if ((dirp = opendir(path.c_str())) == nullptr) {
        std::cout << "Couldn't open "<<path<<"\n";
        return;
    }

    int wd = inotify_watch_dir(path.c_str());
    struct stat file_stat{};
    int status;

    inotify_check_events();
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
    
            auto fe=entryPool.create_entry(fileCount,dp->d_name, isDir ? FileEntry::DIRECTORY : FileEntry::FILE);
            fe->set_size(file_stat.st_size);
    
            mtx.lock();
            fe->set_parent(parent);
            ++fileCount;
            mtx.unlock();
            
            if(isDir)
            {
                std::string newPath;
                fe->get_path(newPath);
                if(newPath.back() != '/')
                    newPath.append("/");
                if(!in_array(newPath, availableRoots) && !in_array(newPath, excludedMounts))
                    scan_dir_prv(fe);
                else
                    std::cout<<"Skip scan of: "<<newPath<<"\n";
            }


//            std::cout <<"dir: "<<isDir<<", "<<path_buf<<", "<< dp->d_name<<", "<<file_stat.st_size<<"\n";
        }

        if(scannerStatus!=SCANNING)
            break;
    }

    //TODO set up some internal limit (changed from settings) for amount of descriptors
    // and only if we're at limit - remove existing watch from the smallest directory
    //don't watch small directories
    if(parent->get_size()<1024*1024*100)
        inotify_rm_watch(SpaceScannerPrv::inotifyFd, wd);
    else if(wd!=-1){
        SpaceScannerPrv::inotifyWdMap[wd]=parent;
        ++SpaceScannerPrv::watchedDirs;
    }

    hasPendingChanges = true;
//    do {
//        if ((dp = readdir(dirp)) != nullptr) {
//            std::cout << dp->d_name<<"\n";
//
//        }
//    } while (dp != nullptr);
    
    
    
    closedir(dirp);
    
}

#endif

