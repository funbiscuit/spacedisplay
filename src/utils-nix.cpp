
#ifdef __linux__

#include <iostream>
#include <regex>
#include <cstdio>
#include <memory>
#include <fstream>
#include <stdexcept>
#include <string>
#include <array>
#include <dirent.h>
#include <cstring>
#include <unistd.h>
#include <pwd.h>
#include <thread>

#include "utils.h"

namespace Utils
{
    enum FileManager
    {
        FM_UNKNOWN=0,
        FM_DOLPHIN=1,
        FM_NAUTILUS=2,
        FM_NEMO=3
    };

    void system_async(std::string cmd);
}

void system_async(std::string cmd)
{
    cmd = regex_replace(cmd, std::regex("[$]"), "\\$");
    std::thread t1([cmd] {
        std::cout << "Executing: "<<cmd<<"\n";
        system(cmd.c_str());
    });
    t1.detach();
}

std::string exec(const char* cmd) {
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


void open_folder_in_file_manager(const char* folder_path)
{
    auto command = string_format("xdg-open \"%s\"", folder_path);
    system_async(command);
}

int parse_fm_desktop_file(const std::string &path)
{
    int result = Utils::FM_UNKNOWN;

    std::ifstream infile(path);
    std::string line;
    while (std::getline(infile, line))
    {
        if(line.rfind("Exec=", 0)==0)
        {
            if(line.find("dolphin")!=std::string::npos)
            {
                result = Utils::FM_DOLPHIN;
                break;
            }
            else if(line.find("nautilus")!=std::string::npos)
            {
                result = Utils::FM_NAUTILUS;
                break;
            }
            else if(line.find("nemo")!=std::string::npos)
            {
                result = Utils::FM_NEMO;
                break;
            }

        }
    }

    return result;
}

int get_default_manager()
{
    const char *homedir;

    if ((homedir = getenv("HOME")) == nullptr) {
        homedir = getpwuid(getuid())->pw_dir;
    }

    std::vector<std::string> desktopLocations;
    desktopLocations.push_back(string_format("%s/.local/share/applications", homedir));
    desktopLocations.emplace_back("/usr/local/share/applications");
    desktopLocations.emplace_back("/usr/share/applications");

    auto fileManager = exec("xdg-mime query default inode/directory");
    auto fileManagerEnd = fileManager.find_first_of('\n');
    if(fileManagerEnd != std::string::npos)
    {
        fileManager=fileManager.substr(0,fileManagerEnd);
    }
    std::cout << "Default file manager: "<< fileManager<<"\n";


    for(auto& loc : desktopLocations)
    {
        DIR *dirp;
        struct dirent *dp;

        if ((dirp = opendir(loc.c_str())) != nullptr) {
            while((dp = readdir(dirp)) != nullptr) {
                if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
                    continue;

                if(strcmp(dp->d_name, fileManager.c_str())==0)
                {
                    auto path = string_format("%s/%s", loc.c_str(), dp->d_name);
                    return parse_fm_desktop_file(path);
                }
            }
        }
    }

    return Utils::FM_UNKNOWN;
}

// on Linux we try to determine default file manager to use appropriate command
// to select file. From most popular managers only thunar is not supported (it seems it doesn't have
// command for selecting file
//
// If file manager was not detected than we just open parent directory of this file (file itself is not selected)
void show_file_in_file_manager(const char* file_path)
{
    std::string command;
    switch (get_default_manager())
    {
        case Utils::FM_DOLPHIN:
            command = string_format("dolphin --select \"%s\"", file_path);
            break;
        case Utils::FM_NAUTILUS:
            command = string_format("nautilus --select \"%s\"", file_path);
            break;
        case Utils::FM_NEMO:
            command = string_format("nemo \"%s\"", file_path);
            break;
        case Utils::FM_UNKNOWN:
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

#endif
