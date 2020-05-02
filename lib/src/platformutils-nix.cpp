#include "platformutils.h"
#include "utils.h"

#include <fstream>
#include <regex>
#include <iostream>
#include <thread>
#include <memory>
#include <array>
#include <cstdio>

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>

namespace PlatformUtils
{
    enum class FileManager
    {
        UNKNOWN=0,
        DOLPHIN=1,
        NAUTILUS=2,
        NEMO=3
    };

    void system_async(std::string cmd);

    std::string exec(const char* cmd);

    FileManager parse_fm_desktop_file(const std::string &path);

    FileManager get_default_manager();
}


class FileIteratorPlatform
{
public:
    // can't copy
    FileIteratorPlatform(const FileIterator&) = delete;
    FileIteratorPlatform& operator= (const FileIterator&) = delete;
    ~FileIteratorPlatform()
    {
        if(dirp!=nullptr)
            closedir(dirp);
    }

    explicit FileIteratorPlatform(const std::string& path_) :
            path(path_), isValid(false), name(""), isDir(false), size(0), dirp(nullptr)
    {
        dirp = opendir(path.c_str());
        get_next_file_data();
    }

    bool isValid;
    std::string name;
    bool isDir;
    int64_t size;

    FileIteratorPlatform& operator++ ()
    {
        get_next_file_data();
        return *this;
    }
private:

    DIR *dirp;
    const std::string& path;

    /**
     * Gets file data for the next file returned by handle
     * If handle is new (just returned by opendir) that filedata for the first file will be read
     */
    void get_next_file_data()
    {
        isValid = false;
        if(dirp == nullptr)
            return;

        struct stat file_stat{};
        struct dirent *dp;

        while((dp = readdir(dirp)) != nullptr &&
              (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0));

        if(dp != nullptr)
        {
            auto nameLen=strlen(dp->d_name);

            // to get file info we need full path
            std::string child = path;
            child.append("/");
            child.append( dp->d_name);

            if(lstat(child.c_str(), &file_stat)==0)
            {
                isValid = true;
                isDir=S_ISDIR(file_stat.st_mode);
                name = dp->d_name;
                size = file_stat.st_size;
            }
        }
    }
};

FileIterator::FileIterator(const std::string& path)
{
    pFileIterator = Utils::make_unique<FileIteratorPlatform>(path);
    update();
}

FileIterator::~FileIterator() = default;

FileIterator &FileIterator::operator++()
{
    ++(*pFileIterator);
    update();
    return *this;
}

void FileIterator::update()
{
    isValid=pFileIterator->isValid;
    name=pFileIterator->name;
    isDir=pFileIterator->isDir;
    size=pFileIterator->size;
}


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

            //include closing slash if it isn't there
            if(mount.back() != PlatformUtils::filePathSeparator)
                mount.push_back(PlatformUtils::filePathSeparator);
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


void PlatformUtils::system_async(std::string cmd)
{
    cmd = regex_replace(cmd, std::regex("[$]"), "\\$");
    std::thread t1([cmd] {
        std::cout << "Executing: "<<cmd<<"\n";
        system(cmd.c_str());
    });
    t1.detach();
}

std::string PlatformUtils::exec(const char* cmd)
{
    std::array<char, 128> buffer{};
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

void PlatformUtils::open_folder_in_file_manager(const char* folder_path)
{
    auto command = string_format("xdg-open \"%s\"", folder_path);
    system_async(command);
}

PlatformUtils::FileManager PlatformUtils::parse_fm_desktop_file(const std::string &path)
{
    auto result = FileManager::UNKNOWN;

    std::ifstream infile(path);
    std::string line;
    while (std::getline(infile, line))
    {
        if(line.rfind("Exec=", 0)==0)
        {
            if(line.find("dolphin")!=std::string::npos)
            {
                result = FileManager::DOLPHIN;
                break;
            }
            else if(line.find("nautilus")!=std::string::npos)
            {
                result = FileManager::NAUTILUS;
                break;
            }
            else if(line.find("nemo")!=std::string::npos)
            {
                result = FileManager::NEMO;
                break;
            }
        }
    }

    return result;
}

PlatformUtils::FileManager PlatformUtils::get_default_manager()
{
    const char *homedir;

    if ((homedir = getenv("HOME")) == nullptr)
        homedir = getpwuid(getuid())->pw_dir;


    std::vector<std::string> desktopLocations;
    desktopLocations.push_back(string_format("%s/.local/share/applications", homedir));
    desktopLocations.emplace_back("/usr/local/share/applications");
    desktopLocations.emplace_back("/usr/share/applications");

    auto fileManager = exec("xdg-mime query default inode/directory");
    auto fileManagerEnd = fileManager.find_first_of('\n');
    if(fileManagerEnd != std::string::npos)
        fileManager=fileManager.substr(0,fileManagerEnd);
    if(fileManager.empty())
        return FileManager::UNKNOWN;
    std::cout << "Default file manager: "<< fileManager<<"\n";


    for(auto& loc : desktopLocations)
    {
        for(FileIterator it(loc); it.is_valid(); ++it)
        {
            if(it.name == fileManager)
            {
                auto path = string_format("%s/%s", loc.c_str(), it.name.c_str());
                return parse_fm_desktop_file(path);
            }
        }
    }

    return FileManager::UNKNOWN;
}

// on Linux we try to determine default file manager to use appropriate command
// to select file. From most popular managers only thunar is not supported (it seems it doesn't have
// command for selecting file)
//
// If file manager was not detected than we just open parent directory of this file (file itself is not selected)
void PlatformUtils::show_file_in_file_manager(const char* file_path)
{
    std::string command;
    switch (get_default_manager())
    {
        case FileManager::DOLPHIN:
            command = string_format("dolphin --select \"%s\"", file_path);
            break;
        case FileManager::NAUTILUS:
            command = string_format("nautilus --select \"%s\"", file_path);
            break;
        case FileManager::NEMO:
            command = string_format("nemo \"%s\"", file_path);
            break;
        case FileManager::UNKNOWN:
        default:
            std::string dir_path = file_path;
            auto last_slash = dir_path.find_last_of('/');
            if(last_slash != std::string::npos)
                dir_path = dir_path.substr(0,last_slash);
            command = string_format("xdg-open \"%s\"", dir_path.c_str());
            break;
    }

    system_async(command);
}

bool PlatformUtils::deleteDir(const std::string &path)
{
    // not implemented
    return false;
}
