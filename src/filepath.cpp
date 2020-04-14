
#include <algorithm>
#include <iostream>
#include "filepath.h"
#include "platformutils.h"


FilePath::FilePath(const std::string& root_)
{
    std::string root = root_;
    
    // replaces all incorrect slashes if any and adds slash at the end if it wasn't there
    normalize(root, true);
    
    if(root.empty())
    {
        std::cerr << "Can't create path from empty root!\n";
        return;
    }
    
    parts.push_back(root);
}

std::string FilePath::getPath(bool addDirSlash)
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

std::string FilePath::getRoot()
{
    if(parts.empty())
    {
        std::cerr << "Attempting to get path root for empty path!\n";
        return "";
    }
    
    return parts[0];
}

bool FilePath::addDir(const std::string& name)
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
    return true;
}

bool FilePath::addFile(const std::string& name)
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
    return true;
}

bool FilePath::isDir()
{
    return parts.empty() ? false : (parts.back().back() == PlatformUtils::filePathSeparator);
}

bool FilePath::canGoUp()
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
    
    char prevChar = '\0';
    
    auto it = path.begin();
    
    // remove all repeating slashes
    while (it != path.end())
    {
        char c = *it;
        if(prevChar == c && c == PlatformUtils::filePathSeparator)
            it=path.erase(it);
        else
            ++it;
        prevChar = c;
    }
    
    if(!keepTrailingSlash && prevChar == PlatformUtils::filePathSeparator)
        path.pop_back();
    else if(keepTrailingSlash && prevChar != PlatformUtils::filePathSeparator)
        path.push_back(PlatformUtils::filePathSeparator);
}
