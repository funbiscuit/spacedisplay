#ifdef __linux__
#include "platformutils.h"
#include "utils.h"

#include <fstream>
#include <regex>

#include <sys/stat.h>

bool PlatformUtils::can_scan_dir(const std::string& path)
{
    struct stat file_stat{};
    if(lstat(path.c_str(), &file_stat)==0)
        return S_ISDIR(file_stat.st_mode);
    else
        return false;
}

void PlatformUtils::get_mount_points(std::vector<std::string>& availableMounts, std::vector<std::string>& excludedMounts)
{
    availableMounts.clear();
    excludedMounts.clear();

    //TODO add more partitions if supported
    const std::vector<std::string> partitions = {"ext2","ext3","ext4","vfat","ntfs","fuseblk"};

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
            if(Utils::in_array(part, partitions))
            {
                availableMounts.push_back(mount);
            } else{
                excludedMounts.push_back(mount);
            }
        }
    }
}

bool PlatformUtils::get_mount_space(const std::string& path, uint64_t& totalSpace, uint64_t& availableSpace)
{
    struct statvfs disk_stat;
    totalSpace = 0;
    availableSpace = 0;
    if(statvfs(path.c_str(), &disk_stat)==0)
    {
        totalSpace=uint64_t(disk_stat.f_frsize)*uint64_t(disk_stat.f_blocks);
        availableSpace=uint64_t(disk_stat.f_frsize)*uint64_t(disk_stat.f_bavail);
        return true;
    }
    else
        return false;
}

#endif
