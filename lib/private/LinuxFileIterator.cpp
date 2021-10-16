#include "platformutils.h"

#include "LinuxFileIterator.h"

#include <sys/stat.h>

LinuxFileIterator::LinuxFileIterator(std::string path) :
        valid(false), dir(false), size(0), path(std::move(path)) {
    dirp = opendir(this->path.c_str());
    getNextFileData();
}

LinuxFileIterator::~LinuxFileIterator() {
    if (dirp != nullptr) {
        closedir(dirp);
    }
}

std::unique_ptr<FileIterator> FileIterator::create(const std::string &path) {
    return std::unique_ptr<LinuxFileIterator>(new LinuxFileIterator(path));
}

void LinuxFileIterator::operator++() {
    assertValid();
    getNextFileData();
}

bool LinuxFileIterator::isValid() const {
    return valid;
}

const std::string &LinuxFileIterator::getName() const {
    assertValid();
    return name;
}

bool LinuxFileIterator::isDir() const {
    assertValid();
    return dir;
}

int64_t LinuxFileIterator::getSize() const {
    assertValid();
    return size;
}

void LinuxFileIterator::getNextFileData() {
    valid = false;
    if (dirp == nullptr)
        return;

    struct stat file_stat{};
    struct dirent *dp;

    while ((dp = readdir(dirp)) != nullptr) {
        name = dp->d_name;
        if (name.empty() || name == "." || name == "..")
            continue;
        break;
    }

    if (dp == nullptr)
        return;

    // to get file info we need full path
    std::string child = path;
    child.push_back('/');
    child.append(name);

    if (lstat(child.c_str(), &file_stat) == 0) {
        valid = true;
        dir = S_ISDIR(file_stat.st_mode);
        size = file_stat.st_size;
    }
}
