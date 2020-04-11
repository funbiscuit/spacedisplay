#ifdef _WIN32
#include "platformutils.h"

#include <Windows.h>

bool PlatformUtils::can_scan_dir(const std::string& path)
{
    auto wpath=PlatformUtils::str2wstr(path);
    bool result = false;

    auto handle=CreateFileW(
            wpath.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            nullptr
    );

    if(handle!=INVALID_HANDLE_VALUE)
    {
        BY_HANDLE_FILE_INFORMATION lpFileInformation;
        if(GetFileInformationByHandle(handle, &lpFileInformation))
            result = (lpFileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
    else
        return false;

    CloseHandle(handle);
    return result;
}

void PlatformUtils::get_mount_points(std::vector<std::string>& availableMounts, std::vector<std::string>& excludedMounts)
{
    availableMounts.clear();
    excludedMounts.clear();  //on windows it is safe to scan any fixed or removable drive

    auto drivesMask=GetLogicalDrives();

    char driveName[4] = {'A', ':', '\\', '\0'};

    for(char drive='A';drive<='Z';++drive)
    {
        if(drivesMask & 0x1u)
        {
            driveName[0]=drive;
            auto type=GetDriveTypeA(driveName);

            if(type==DRIVE_FIXED || type==DRIVE_REMOVABLE)
                availableMounts.emplace_back(driveName);
        }
        drivesMask>>=1u;
    }
}

bool PlatformUtils::get_mount_space(const std::string& path, uint64_t& totalSpace, uint64_t& availableSpace)
{
    auto wpath=PlatformUtils::str2wstr(path);

    ULARGE_INTEGER totalBytes;
    ULARGE_INTEGER freeBytesForCaller;

    totalSpace = 0;
    availableSpace = 0;

    if(GetDiskFreeSpaceExW(wpath.c_str(), &freeBytesForCaller, &totalBytes, nullptr) != 0)
    {
        totalSpace=totalBytes.QuadPart;
        availableSpace=freeBytesForCaller.QuadPart;

        return true;
    }
    else
        return false;
}

std::wstring PlatformUtils::str2wstr(std::string const &str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring ret(len, '\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), (LPWSTR)ret.data(), (int)ret.size());
    return ret;
}

std::string PlatformUtils::wstr2str(std::wstring const &wstr)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string ret(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), (LPSTR)ret.data(), (int)ret.size(), nullptr, nullptr);
    return ret;
}

#endif
