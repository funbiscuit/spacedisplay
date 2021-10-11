#include "FileManager.h"
#include "FileIterator.h"

#include "utils.h"
#include "filepath.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <regex>

#include <unistd.h>
#include <pwd.h>

enum class FileManagerType {
    UNKNOWN = 0,
    DOLPHIN = 1,
    NAUTILUS = 2,
    NEMO = 3,
};

/**
 * Parses desktop file at the specified path and
 * tries to detect which file manager it corresponds to
 * @param path
 * @return
 */
static FileManagerType parseFileManagerDesktopFile(const std::string &path);

/**
 * Uses "xdg-mime query default inode/directory" to get default file manager
 * @return
 */
static FileManagerType getDefaultFileManager();

static void systemAsync(std::string cmd);

static std::string exec(const char *cmd);

void FileManager::openFolder(const std::string &path) {
    std::string folder_path = path;
    FilePath::normalize(folder_path, FilePath::SlashHandling::ADD);
    auto command = Utils::strFormat("xdg-open \"%s\"", folder_path.c_str());
    systemAsync(command);
}

/**
 * on Linux we try to determine default file manager to use appropriate command
 * to select file. From most popular managers only thunar is not supported (it seems it doesn't have
 * command for selecting file)
 *
 * If file manager was not detected than we just open parent directory of this file (file itself is not selected)
 */
void FileManager::showFile(const std::string &path) {
    std::string file_path = path;
    FilePath::normalize(file_path, FilePath::SlashHandling::REMOVE);
    std::string command;
    switch (getDefaultFileManager()) {
        case FileManagerType::DOLPHIN:
            command = Utils::strFormat("dolphin --select \"%s\"", file_path.c_str());
            break;
        case FileManagerType::NAUTILUS:
            command = Utils::strFormat("nautilus --select \"%s\"", file_path.c_str());
            break;
        case FileManagerType::NEMO:
            command = Utils::strFormat("nemo \"%s\"", file_path.c_str());
            break;
        case FileManagerType::UNKNOWN:
        default:
            std::string dir_path = file_path;
            auto last_slash = dir_path.find_last_of('/');
            if (last_slash != std::string::npos)
                dir_path = dir_path.substr(0, last_slash);
            command = Utils::strFormat("xdg-open \"%s\"", dir_path.c_str());
            break;
    }

    systemAsync(command);
}

FileManagerType parseFileManagerDesktopFile(const std::string &path) {
    std::ifstream infile(path);
    std::string line;
    while (std::getline(infile, line)) {
        if (line.rfind("Exec=", 0) != 0)
            continue;

        if (line.find("dolphin") != std::string::npos) {
            return FileManagerType::DOLPHIN;
        } else if (line.find("nautilus") != std::string::npos) {
            return FileManagerType::NAUTILUS;
        } else if (line.find("nemo") != std::string::npos) {
            return FileManagerType::NEMO;
        }
    }

    return FileManagerType::UNKNOWN;
}

FileManagerType getDefaultFileManager() {
    const char *homedir;

    if ((homedir = getenv("HOME")) == nullptr) {
        homedir = getpwuid(getuid())->pw_dir;
    }

    std::vector<std::string> desktopLocations;
    desktopLocations.push_back(Utils::strFormat("%s/.local/share/applications", homedir));
    desktopLocations.emplace_back("/usr/local/share/applications");
    desktopLocations.emplace_back("/usr/share/applications");

    auto fileManager = exec("xdg-mime query default inode/directory");
    auto fileManagerEnd = fileManager.find_first_of('\n');
    if (fileManagerEnd != std::string::npos) {
        fileManager = fileManager.substr(0, fileManagerEnd);
    }
    if (fileManager.empty()) {
        return FileManagerType::UNKNOWN;
    }
    std::cout << "Default file manager: " << fileManager << "\n";

    for (auto &loc: desktopLocations) {
        for (auto it = FileIterator::create(loc); it->isValid(); ++(*it)) {
            if (it->getName() == fileManager) {
                auto path = Utils::strFormat("%s/%s", loc.c_str(), it->getName().c_str());
                return parseFileManagerDesktopFile(path);
            }
        }
    }

    return FileManagerType::UNKNOWN;
}

void systemAsync(std::string cmd) {
    cmd = regex_replace(cmd, std::regex("[$]"), "\\$");
    std::thread t1([cmd] {
        std::cout << "Executing: " << cmd << "\n";
        system(cmd.c_str());
    });
    t1.detach();
}

std::string exec(const char *cmd) {
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
