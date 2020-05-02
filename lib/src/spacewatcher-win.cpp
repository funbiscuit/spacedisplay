#include "spacewatcher-win.h"

#include "platformutils.h"
#include "utils.h"

#include <iostream>

SpaceWatcherWin::SpaceWatcherWin() : watchedDir(INVALID_HANDLE_VALUE)
{
    watchBuffer=std::unique_ptr<DWORD[]>(new DWORD[watchBufferSize]);
    startThread();
}
SpaceWatcherWin::~SpaceWatcherWin()
{
    // endWatch() is virtual so we can't call it in destructor
    _endWatch();
    stopThread();
}

bool SpaceWatcherWin::beginWatch(const std::string& path)
{
    endWatch();

    auto wname=PlatformUtils::str2wstr(path);
    watchedDir=CreateFileW(
            wname.c_str(),
            FILE_LIST_DIRECTORY,  //needed for reading changes
            FILE_SHARE_READ | FILE_SHARE_WRITE| FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, //for async use
            nullptr
    );

    if(watchedDir != INVALID_HANDLE_VALUE)
    {
        watchedPath = path;
        return true;
    }
    return false;
}

void SpaceWatcherWin::endWatch()
{
    _endWatch();
}

void SpaceWatcherWin::_endWatch()
{
    if(watchedDir != INVALID_HANDLE_VALUE)
    {
        CloseHandle(watchedDir);
        watchedPath.clear();
        watchedDir = INVALID_HANDLE_VALUE;
    }
}

bool SpaceWatcherWin::isWatching()
{
    return watchedDir != INVALID_HANDLE_VALUE;
}

void SpaceWatcherWin::readEvents()
{
    if(!isWatching())
        return;

    DWORD bytesReturned=0;

    auto success=ReadDirectoryChangesW(
            watchedDir,
            watchBuffer.get(), //DWORD aligned buffer
            watchBufferSize*sizeof(DWORD), //size of buffer in bytes
            true, //watching all files recursively
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE,
            &bytesReturned,
            nullptr,
            nullptr
    );
    if(success == 0 || bytesReturned == 0)
        return;

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
            auto notifyInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&watchBuffer[currentDword]);

            auto fileEvent = Utils::make_unique<FileEvent>();
            switch (notifyInfo->Action)
            {
                case FILE_ACTION_ADDED:
                    fileEvent->action=FileAction::ADDED;
                    break;
                case FILE_ACTION_REMOVED:
                    fileEvent->action=FileAction::REMOVED;
                    break;
                case FILE_ACTION_MODIFIED:
                    fileEvent->action=FileAction::MODIFIED;
                    break;
                case FILE_ACTION_RENAMED_OLD_NAME:
                    fileEvent->action=FileAction::OLD_NAME;
                    break;
                case FILE_ACTION_RENAMED_NEW_NAME:
                    fileEvent->action=FileAction::NEW_NAME;
                    break;
                default:
                    continue;//should not happen
            }

            auto nameLen= notifyInfo->FileNameLength / sizeof(wchar_t);
            //nameLen doesn't include null-terminating byte
            auto name=Utils::make_unique_arr<wchar_t>(nameLen+1);

            memcpy(name.get(), notifyInfo->FileName, notifyInfo->FileNameLength);
            name[nameLen]=L'\0';
            fileEvent->filepath= watchedPath + PlatformUtils::wstr2str(name.get());

            // returned filename might (or might not) be a short path. we should restore long path

            bool pathConverted = false;
            if(fileEvent->action != FileAction::MODIFIED && fileEvent->action != FileAction::REMOVED)
            {
                PlatformUtils::toLongPath(fileEvent->filepath);
            }

            if(fileEvent->filepath.back() == '\\')
                fileEvent->filepath.pop_back();

            auto lastSlash = fileEvent->filepath.find_last_of('\\');
            if(lastSlash != std::string::npos)
                fileEvent->parentpath = fileEvent->filepath.substr(0, lastSlash+1);
            else
                fileEvent->parentpath = "";

            PlatformUtils::toLongPath(fileEvent->parentpath);

            addEvent(std::move(fileEvent));

            //offset to next this should be dword aligned
            auto next = notifyInfo->NextEntryOffset / sizeof(DWORD);
            currentDword+=next;
            if(next==0)
                break;
        }
    }

}
