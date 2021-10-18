#include "platformutils.h"
#include "utils.h"

#include <iostream>

#include <Windows.h>

bool PlatformUtils::can_scan_dir(const std::string &path) {
    auto wpath = PlatformUtils::str2wstr(path);
    bool result = false;

    auto handle = CreateFileW(
            wpath.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            nullptr
    );

    if (handle != INVALID_HANDLE_VALUE) {
        BY_HANDLE_FILE_INFORMATION lpFileInformation;
        if (GetFileInformationByHandle(handle, &lpFileInformation))
            result = (lpFileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    } else
        return false;

    CloseHandle(handle);
    return result;
}

std::vector<std::string> PlatformUtils::getAvailableMounts() {
    std::vector<std::string> availableMounts;

    auto drivesMask = GetLogicalDrives();

    char driveName[4] = {'A', ':', '\\', '\0'};

    for (char drive = 'A'; drive <= 'Z'; ++drive) {
        if (drivesMask & 0x1u) {
            driveName[0] = drive;
            auto type = GetDriveTypeA(driveName);

            if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE)
                availableMounts.emplace_back(driveName);
        }
        drivesMask >>= 1u;
    }

    return availableMounts;
}

std::vector<std::string> PlatformUtils::getExcludedPaths() {
    return {};
}

bool PlatformUtils::get_mount_space(const std::string &path, int64_t &totalSpace, int64_t &availableSpace) {
    auto wpath = PlatformUtils::str2wstr(path);

    ULARGE_INTEGER totalBytes;
    ULARGE_INTEGER freeBytesForCaller;

    totalSpace = 0;
    availableSpace = 0;

    if (GetDiskFreeSpaceExW(wpath.c_str(), &freeBytesForCaller, &totalBytes, nullptr) != 0) {
        totalSpace = static_cast<int64_t>(totalBytes.QuadPart);
        availableSpace = static_cast<int64_t>(freeBytesForCaller.QuadPart);

        return true;
    } else
        return false;
}

std::wstring PlatformUtils::str2wstr(std::string const &str) {
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int) str.size(), nullptr, 0);
    std::wstring ret(len, '\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int) str.size(), (LPWSTR) ret.data(), (int) ret.size());
    return ret;
}

std::string PlatformUtils::wstr2str(std::wstring const &wstr) {
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int) wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string ret(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int) wstr.size(), (LPSTR) ret.data(), (int) ret.size(), nullptr,
                        nullptr);
    return ret;
}

bool PlatformUtils::toLongPath(std::string &shortPath) {
    auto wpath = PlatformUtils::str2wstr(shortPath);
    auto requiredBufLen = GetLongPathNameW(wpath.c_str(), nullptr, 0);
    if (requiredBufLen > 0) {
        auto longName = Utils::make_unique_arr<wchar_t>(requiredBufLen);
        auto copiedChars = GetLongPathNameW(wpath.c_str(), longName.get(), requiredBufLen);
        if (copiedChars < requiredBufLen && copiedChars > 0) {
            shortPath = PlatformUtils::wstr2str(longName.get());
            return true;
        }
    }
    return false;
}

bool PlatformUtils::deleteDir(const std::string &path) {
    std::wstring wpath = str2wstr(path);

    // string in SHFILEOPSTRUCTW must be double null terminated so do it manually
    std::vector<wchar_t> buffer(wpath.size() + 2, L'\0');
    memcpy(buffer.data(), wpath.c_str(), wpath.size() * sizeof(wchar_t));

    SHFILEOPSTRUCTW file_op = {
            nullptr,
            FO_DELETE,
            buffer.data(),
            nullptr,
            FOF_NOCONFIRMATION |
            FOF_NOERRORUI |
            FOF_SILENT,
            false,
            nullptr,
            nullptr};
    return SHFileOperationW(&file_op) == 0; // returns 0 on success, non zero on failure.
}
