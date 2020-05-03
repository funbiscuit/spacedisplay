#include "DirHelper.h"
#include "platformutils.h"
#include "utils.h"
#include "filepath.h"

#include <cstdio>

//TODO rewrite
#if _WIN32
#include <direct.h>
#define mkdir(dir, mode) _mkdir(dir)
#define rmdir(dir) _rmdir(dir)
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif


DirHelper::DirHelper(const std::string& path)
{
    // delete dir if it already exist
    PlatformUtils::deleteDir(path);
    if(mkdir(path.c_str(), 0755) == 0)
        root = Utils::make_unique<FilePath>(path);
}

DirHelper::~DirHelper()
{
    if(root)
        PlatformUtils::deleteDir(root->getRoot());
}

bool DirHelper::createDir(const std::string& path)
{
    if(!root)
        return false;

    auto newPath = *root;
    newPath.addDir(path);

    return mkdir(newPath.getPath().c_str(), 0755) == 0;
}

bool DirHelper::createFile(const std::string& path)
{
    if(!root)
        return false;

    auto newPath = *root;
    newPath.addFile(path);

    auto f = fopen(newPath.getPath().c_str(), "w+");
    if(f)
    {
        fclose(f);
        return true;
    }
    return false;
}

bool DirHelper::deleteDir(const std::string& path)
{
    if(!root)
        return false;

    auto newPath = *root;
    newPath.addDir(path);

    return PlatformUtils::deleteDir(newPath.getPath());
}

bool DirHelper::rename(const std::string& oldPath, const std::string& newPath)
{
    if(!root)
        return false;

    auto path = *root;
    path.addFile(oldPath); //this way it works for both dirs and files
    auto oldStr = path.getPath(false);
    path.goUp();
    path.addFile(newPath);

    return ::rename(oldStr.c_str(), path.getPath(false).c_str()) == 0;
}
