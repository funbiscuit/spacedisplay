
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


void SpaceScanner::init_platform()
{
}

void SpaceScanner::cleanup_platform()
{
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

    while((dp = readdir(dirp)) != nullptr && scannerStatus != ScannerStatus::STOPPING)
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

