#include "platformutils.h"

#include "WinFileIterator.h"

#include <stdexcept>

WinFileIterator::WinFileIterator(const std::string &path) :
        valid(false), dir(false), size(0), dirHandle(INVALID_HANDLE_VALUE) {
    auto wname = PlatformUtils::str2wstr(path);
    //it's okay to have multiple slashes at the end
    wname.append(L"\\*");

    WIN32_FIND_DATAW fileData;

    dirHandle = FindFirstFileW(wname.c_str(), &fileData);
    processFileData(true, &fileData);
}

WinFileIterator::~WinFileIterator() {
    if (dirHandle != INVALID_HANDLE_VALUE) {
        FindClose(dirHandle);
    }
}

std::unique_ptr<FileIterator> FileIterator::create(const std::string &path) {
    return std::unique_ptr<WinFileIterator>(new WinFileIterator(path));
}

void WinFileIterator::operator++() {
    assertValid();
    WIN32_FIND_DATAW fileData;

    processFileData(false, &fileData);
}

bool WinFileIterator::isValid() const {
    return valid;
}

const std::string &WinFileIterator::getName() const {
    assertValid();
    return name;
}

bool WinFileIterator::isDir() const {
    assertValid();
    return dir;
}

int64_t WinFileIterator::getSize() const {
    assertValid();
    return size;
}

void WinFileIterator::processFileData(bool isFirst, WIN32_FIND_DATAW *fileData) {
    bool found = dirHandle != INVALID_HANDLE_VALUE;

    if (found && (isFirst || FindNextFileW(dirHandle, fileData) != 0)) {
        auto cname = PlatformUtils::wstr2str(fileData->cFileName);

        while (found && (cname.empty() || cname == "." || cname == "..")) {
            found = FindNextFileW(dirHandle, fileData) != 0;
            cname = PlatformUtils::wstr2str(fileData->cFileName);
        }

        if (found) {
            valid = true;
            name = std::move(cname);
            dir = (fileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            //don't count mount points and symlinks as directories
            if ((fileData->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0 &&
                (fileData->dwReserved0 == IO_REPARSE_TAG_SYMLINK ||
                 fileData->dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT)) {
                dir = false;
            }

            size = (int64_t(fileData->nFileSizeHigh) * (int64_t(MAXDWORD) + 1)) +
                   int64_t(fileData->nFileSizeLow);
        }
    } else {
        valid = false;
    }
}
