#include "spacewatcher.h"

#include "platformutils.h"
#include "utils.h"

#include <iostream>

#include <Windows.h>


void SpaceWatcher::watchDir(const std::string& path)
{
    auto wname=PlatformUtils::str2wstr(path);
    auto handle=CreateFileW(
            wname.c_str(),
            FILE_LIST_DIRECTORY,  //needed for reading changes
            FILE_SHARE_READ | FILE_SHARE_WRITE| FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, //for async use
            nullptr
    );

    std::cout<<"Start monitoring changes\n";
    int bufSize=1024*32;
    auto buffer=std::unique_ptr<DWORD[]>(new DWORD[bufSize]);
    while(true)
    {
        DWORD bytesReturned=0;

        auto success=ReadDirectoryChangesW(
                handle,
                buffer.get(), //DWORD aligned buffer
                bufSize, //size of buffer in bytes
                true, //watching all files recursively
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE,
                &bytesReturned,
                nullptr,
                nullptr
        );
        if(success == 0)
            break;

        //calculate required number of dwords to hold returned bytes
        auto dwords=bytesReturned/sizeof(DWORD)+1;

        // buffer is filled with objects of structure FILE_NOTIFY_INFORMATION
        // first DWORD holds offset to the next record (in bytes!)
        // second DWORD holds Action
        // third DWORD holds filename length
        // then wchar buffer of filename length (without null terminating char)

        // minimum size is 4, so do nothing until we receive it
        if(dwords>3)
        {
            int currentDword=0;

            while(true)
            {
                auto notifyInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&buffer[currentDword]);

                FileInfo fileInfo={};
                switch (notifyInfo->Action)
                {
                    case FILE_ACTION_ADDED:
                        fileInfo.Action=FileAction::ADDED;
                        break;
                    case FILE_ACTION_REMOVED:
                        fileInfo.Action=FileAction::REMOVED;
                        break;
                    case FILE_ACTION_MODIFIED:
                        fileInfo.Action=FileAction::MODIFIED;
                        break;
                    case FILE_ACTION_RENAMED_OLD_NAME:
                        fileInfo.Action=FileAction::OLD_NAME;
                        break;
                    case FILE_ACTION_RENAMED_NEW_NAME:
                        fileInfo.Action=FileAction::NEW_NAME;
                        break;
                    default:
                        continue;//should not happen
                }

                auto nameLen= notifyInfo->FileNameLength / sizeof(wchar_t);
                //nameLen doesn't include null-terminating byte
                auto name=Utils::make_unique_arr<wchar_t>(nameLen+1);

                memcpy(name.get(), notifyInfo->FileName, notifyInfo->FileNameLength);
                name[nameLen]=L'\0';
                fileInfo.FileName=path+PlatformUtils::wstr2str(name.get());
                std::cout<<"File name: "<<fileInfo.FileName<<", reason: "<<(int)fileInfo.Action<<"\n";


                // returned filename might (or might not) be a short path. we should restore long path
                // but it is not possible if file was deleted/removed since it will give error file not found
                auto len = GetLongPathNameW(
                        PlatformUtils::str2wstr(fileInfo.FileName).c_str(),
                        nullptr,
                        0
                );
                std::cout<<"Long File name len: "<<len<<"\n";
                if(len==0)
                    std::cout << "err:" << GetLastError() << "\n";


                //offset to next this should be dword aligned
                auto next = notifyInfo->NextEntryOffset / sizeof(DWORD);
                currentDword+=next;
                if(next==0)
                    break;
            }
        }
    }
    std::cout<<"Stop monitoring changes\n";

    CloseHandle(handle);
}
