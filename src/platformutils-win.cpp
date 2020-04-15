#include "platformutils.h"
#include "utils.h"

#include <iostream>

#include <Windows.h>

class FileIteratorPlatform
{
public:
    // can't copy
    FileIteratorPlatform(const FileIterator&) = delete;
    FileIteratorPlatform& operator= (const FileIterator&) = delete;
    ~FileIteratorPlatform()
    {
        if(dirHandle!=INVALID_HANDLE_VALUE)
            FindClose(dirHandle);
    }

    explicit FileIteratorPlatform(const std::string& path) :
            isValid(false), name(""), isDir(false), size(0), dirHandle(INVALID_HANDLE_VALUE)
    {
        auto wname=PlatformUtils::str2wstr(path);
        //it's okay to have multiple slashes at the end
        wname.append(L"\\*");

        WIN32_FIND_DATAW fileData;

        dirHandle=FindFirstFileW(wname.c_str(), &fileData);
        get_file_data(true, &fileData);
    }

    bool isValid;
    std::string name;
    bool isDir;
    int64_t size;

    FileIteratorPlatform& operator++ ()
    {
        WIN32_FIND_DATAW fileData;

        get_file_data(false, &fileData);
        return *this;
    }
private:

    HANDLE dirHandle;

    /**
     * Gets file data for the first or next file returned by handle
     */
    void get_file_data(bool isFirst, WIN32_FIND_DATAW* fileData)
    {
        bool found=dirHandle!=INVALID_HANDLE_VALUE;

        if(found && (isFirst || FindNextFileW(dirHandle, fileData) != 0))
        {
            auto cname=PlatformUtils::wstr2str(fileData->cFileName);

            while(found && (cname == "." || cname == ".."))
            {
                found = FindNextFileW(dirHandle, fileData) != 0;
                cname=PlatformUtils::wstr2str(fileData->cFileName);
            }

            if(found)
            {
                isValid = true;
                name=std::move(cname);
                isDir=(fileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)!=0;
                //don't count mount points and symlinks as directories
                if((fileData->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)!=0 &&
                   (fileData->dwReserved0==IO_REPARSE_TAG_SYMLINK || fileData->dwReserved0==IO_REPARSE_TAG_MOUNT_POINT))
                    isDir=false;

                size=(uint64_t(fileData->nFileSizeHigh) * (uint64_t(MAXDWORD)+1)) + uint64_t(fileData->nFileSizeLow);
            }
        } else
            isValid = false;
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

void PlatformUtils::open_folder_in_file_manager(const char* folder_path)
{
    auto pars = string_format("/n,\"%s\"", folder_path);
    auto parsw = str2wstr(pars);

    std::wcout << L"Launch explorer with args: " << parsw << '\n';

    ShellExecuteW(nullptr, L"open", L"explorer", parsw.c_str(), nullptr, SW_SHOWNORMAL);
}

void PlatformUtils::show_file_in_file_manager(const char* file_path)
{
    auto pars = string_format("/select,\"%s\"", file_path);
    auto parsw = str2wstr(pars);

    std::wcout << L"Launch explorer with args: " << parsw << '\n';

    ShellExecuteW(nullptr, L"open", L"explorer", parsw.c_str(), nullptr, SW_SHOWNORMAL);
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
