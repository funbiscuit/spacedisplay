#include "platformutils.h"
#include "utils.h"
#include "filepath.h"
#include "FileIterator.h"

#include <fstream>
#include <regex>

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

static void processMountPoints(const std::function<void(const std::string &path, bool isExcluded)> &consumer);

bool PlatformUtils::can_scan_dir(const std::string &path) {
    struct stat file_stat{};
    if (lstat(path.c_str(), &file_stat) == 0)
        return S_ISDIR(file_stat.st_mode);
    else
        return false;
}

std::vector<std::string> PlatformUtils::getAvailableMounts() {
    std::vector<std::string> availableMounts;

    processMountPoints([&availableMounts](const std::string &mount, bool isExcluded) {
        if (!isExcluded) {
            availableMounts.push_back(mount);
        }
    });

    return availableMounts;
}

std::vector<std::string> PlatformUtils::getExcludedPaths() {
    std::vector<std::string> excludedPaths;

    processMountPoints([&excludedPaths](const std::string &mount, bool isExcluded) {
        if (isExcluded) {
            excludedPaths.push_back(mount);
        }
    });

    return excludedPaths;
}

bool PlatformUtils::get_mount_space(const std::string &path, uint64_t &totalSpace, uint64_t &availableSpace) {
    struct statvfs disk_stat{};
    totalSpace = 0;
    availableSpace = 0;
    if (statvfs(path.c_str(), &disk_stat) == 0) {
        totalSpace = uint64_t(disk_stat.f_frsize) * uint64_t(disk_stat.f_blocks);
        availableSpace = uint64_t(disk_stat.f_frsize) * uint64_t(disk_stat.f_bavail);
        return true;
    } else
        return false;
}

bool PlatformUtils::deleteDir(const std::string &path) {
    bool deleted = true;
    try {
        FilePath fpath(path);

        for (auto it = FileIterator::create(path); it->isValid(); ++(*it)) {
            if (it->isDir()) {
                fpath.addDir(it->getName());
                deleted &= deleteDir(fpath.getPath());
            } else {
                fpath.addFile(it->getName());
                deleted &= unlink(fpath.getPath().c_str()) == 0;
            }
            fpath.goUp();
        }
        //after all children are removed, remove this directory
        if (deleted)
            deleted &= rmdir(fpath.getPath().c_str()) == 0;
    } catch (std::exception &) {
        return false;
    }

    return deleted;
}

void processMountPoints(const std::function<void(const std::string &, bool)> &consumer) {
    //TODO add more partitions if supported
    const std::vector<std::string> partitions = {"ext2", "ext3", "ext4", "vfat", "ntfs", "fuseblk"};

    std::ifstream file("/proc/mounts");
    std::string str;
    std::regex rgx(R"(^\S* (\S*) (\S*))");
    while (std::getline(file, str)) {
        std::smatch matches;

        if (!std::regex_search(str, matches, rgx) || matches.size() != 3) {
            continue;
        }

        // access path and partition type
        std::string mount = matches[1];
        std::string part = matches[2];

        //include closing slash if it isn't there
        if (mount.back() != PlatformUtils::filePathSeparator) {
            mount.push_back(PlatformUtils::filePathSeparator);
        }
        consumer(mount, !Utils::in_array(part, partitions));
    }
}
