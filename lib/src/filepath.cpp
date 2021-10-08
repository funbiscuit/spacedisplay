
#include <algorithm>
#include <iostream>
#include "filepath.h"
#include "utils.h"
#include "platformutils.h"

FilePath::FilePath(const std::string& root_, uint16_t crc)
{
    if(root_.empty())
        throw std::invalid_argument("Can't set empty root for path");
    std::string root = root_;

    // replaces all incorrect slashes if any and adds slash at the end if it wasn't there
    normalize(root, true);

    parts.clear();
    pathCrcs.clear();
    parts.push_back(root);
    if(crc == 0)
        crc = Utils::strCrc16(root);
    pathCrcs.push_back(crc);
}

FilePath::FilePath(const std::string& path, const std::string& root)
{
    if(path.empty() || root.empty())
        throw std::invalid_argument("Can't create path from empty root or path");

    auto newPath = path;
    auto newRoot = root;
    normalize(newPath, path.back() == PlatformUtils::filePathSeparator ||
                       path.back() == PlatformUtils::invertedFilePathSeparator);
    normalize(newRoot, true);

    if(newPath.rfind(newRoot, 0) != 0)
        throw std::invalid_argument("Provided path doesn't begin with provided root");

    parts.push_back(newRoot);
    pathCrcs.push_back(Utils::strCrc16(newRoot));

    newPath.erase(0, newRoot.length());

    //split remaining string by slashes
    size_t pos;
    while ((pos = newPath.find(PlatformUtils::filePathSeparator)) != std::string::npos)
    {
        std::string dirName = newPath.substr(0, pos + 1); //keep slash
        parts.push_back(dirName);
        dirName.pop_back();// slash should not count in calculation of crc
        pathCrcs.push_back(pathCrcs.back() ^ Utils::strCrc16(dirName));
        newPath.erase(0, pos + 1);
    }
    if(!newPath.empty())
    {
        parts.push_back(newPath);
        pathCrcs.push_back(pathCrcs.back() ^ Utils::strCrc16(newPath));
    }
}

std::string FilePath::getPath(bool addDirSlash) const
{
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
    if(parts.size()==1 || !isDir()) //root and file are returned as is
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
    return parts[0];
}

const std::vector<std::string>& FilePath::getParts() const
{
    return parts;
}

uint16_t FilePath::getPathCrc() const
{
    return pathCrcs.back();
}

void FilePath::addDir(const std::string& name, uint16_t crc)
{
    if(name.empty())
        throw std::invalid_argument("Can't add dir with empty name");
    if(!isDir())
        throw std::invalid_argument("Can't add dir to file path");
    parts.push_back(name);
    // every part that is a directory should have slash at the end
    if(parts.back().back() != PlatformUtils::filePathSeparator)
        parts.back().push_back(PlatformUtils::filePathSeparator);

    if(crc == 0)
        crc = Utils::strCrc16(name);
    pathCrcs.push_back(pathCrcs.back() ^ crc);
}

void FilePath::addFile(const std::string& name, uint16_t crc)
{
    if(name.empty())
        throw std::invalid_argument("Can't add file with empty name");
    if(!isDir())
        throw std::invalid_argument("Can't add file to file path");
    parts.push_back(name);
    if(crc == 0)
        crc = Utils::strCrc16(name);
    pathCrcs.push_back(pathCrcs.back() ^ crc);
}

bool FilePath::isDir() const
{
    return parts.back().back() == PlatformUtils::filePathSeparator;
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
        pathCrcs.pop_back();
        return true;
    } else
        return false;
}

FilePath::CompareResult FilePath::compareTo(const FilePath& path)
{
    if(parts.size() < path.parts.size())
    {
        //this path can be only parent to provided path if crc is the same
        if(path.pathCrcs[parts.size()-1] != pathCrcs.back())
            return CompareResult::DIFFERENT;

        for(int i=0; i<parts.size(); ++i)
            if(parts[i] != path.parts[i])
                return CompareResult::DIFFERENT;

        return CompareResult::PARENT;
    }
    else
    {
        if(pathCrcs[path.parts.size()-1] != path.pathCrcs.back())
            return CompareResult::DIFFERENT;

        for(int i=0; i<path.parts.size(); ++i)
            if(parts[i] != path.parts[i])
                return CompareResult::DIFFERENT;

        if(parts.size() == path.parts.size())
            return CompareResult::EQUAL;

        return CompareResult::CHILD;
    }
}

bool FilePath::makeRelativeTo(const FilePath& parentPath)
{
    auto state = compareTo(parentPath);
    if(state != CompareResult::CHILD) //this path should be child to parentPath
        return false;

    //erase first entries from provided path, so it becomes relative to this path
    parts.erase(parts.begin(), parts.begin()+parentPath.parts.size());
    pathCrcs.erase(pathCrcs.begin(), pathCrcs.begin()+parentPath.parts.size());

    return true;
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
