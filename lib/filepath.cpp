
#include <algorithm>
#include <iostream>
#include "filepath.h"
#include "platformutils.h"

extern "C" {
#include <crc.h>
}

FilePath::FilePath(const std::string& root_, uint16_t crc)
{
    setRoot(root_);
}

void FilePath::setRoot(const std::string& root_, uint16_t crc)
{
    if(root_.empty())
    {
        std::cerr << "Can't create path from empty root!\n";
        return;
    }
    std::string root = root_;
    
    // replaces all incorrect slashes if any and adds slash at the end if it wasn't there
    normalize(root, true);
    
    if(root.empty())
    {
        std::cerr << "Can't create path from empty root!\n";
        return;
    }
    parts.clear();
    partCrcs.clear();
    parts.push_back(root);
    if(crc == 0)
        crc = crc16((char*)root.c_str(), root.length());
    partCrcs.push_back(crc);
}

std::string FilePath::getPath(bool addDirSlash) const
{
    if(parts.empty())
    {
        std::cerr << "Attempting to get path for empty path!\n";
        return "";
    }
    std::string path;
    //each directory name already has slash at the end so we just concatenate them all
    for(const auto& part : parts)
        path.append(part);
    //if we don't need slash at the end for directories - remove it
    if(!addDirSlash && path.back() == PlatformUtils::filePathSeparator)
        path.pop_back();
    
    return path;
}

std::string FilePath::getName() const
{
    if(parts.empty())
        return "";
    else if(parts.size()==1 || !isDir()) //root and file are returned as is
        return parts.back();
    else
    {
        std::string str = parts.back();
        str.pop_back();//remove last slash
        return str;
    }
}

std::string FilePath::getRoot() const
{
    if(parts.empty())
    {
        std::cerr << "Attempting to get path root for empty path!\n";
        return "";
    }
    
    return parts[0];
}

const std::vector<std::string>& FilePath::getParts() const
{
    return parts;
}

const std::vector<uint16_t>& FilePath::getCrs() const
{
    return partCrcs;
}

bool FilePath::addDir(const std::string& name, uint16_t crc)
{
    if(name.empty())
    {
        std::cerr << "Attempting to add empty dir name!\n";
        return false;
    }
    if(!isDir())
    {
        std::cerr << "Attempting to add dir to empty path or to file path!\n";
        return false;
    }
    parts.push_back(name);
    // every part that is a directory should have slash at the end
    if(parts.back().back() != PlatformUtils::filePathSeparator)
        parts.back().push_back(PlatformUtils::filePathSeparator);

    if(crc == 0)
        crc = crc16((char*)name.c_str(), name.length());
    partCrcs.push_back(crc);
    return true;
}

bool FilePath::addFile(const std::string& name, uint16_t crc)
{
    if(name.empty())
    {
        std::cerr << "Attempting to add empty filename!\n";
        return false;
    }
    if(!isDir())
    {
        std::cerr << "Attempting to add file to empty path or to file path!\n";
        return false;
    }
    parts.push_back(name);
    if(crc == 0)
        crc = crc16((char*)name.c_str(), name.length());
    partCrcs.push_back(crc);
    return true;
}

bool FilePath::isDir() const
{
    return parts.empty() ? false : (parts.back().back() == PlatformUtils::filePathSeparator);
}

bool FilePath::canGoUp() const
{
    return parts.size()>1;
}

bool FilePath::goUp()
{
    if(canGoUp())
    {
        parts.pop_back();
        return true;
    } else
        return false;
}

void FilePath::normalize(std::string& path, bool keepTrailingSlash)
{
    // make all slashes correct for this platform
    std::replace(path.begin(), path.end(), PlatformUtils::invertedFilePathSeparator, PlatformUtils::filePathSeparator);

    if(!keepTrailingSlash && path.back() == PlatformUtils::filePathSeparator)
        path.pop_back();
    else if(keepTrailingSlash && path.back() != PlatformUtils::filePathSeparator)
        path.push_back(PlatformUtils::filePathSeparator);
}
