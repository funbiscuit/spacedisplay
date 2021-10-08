#ifndef SPACEDISPLAY_DIRHELPER_H
#define SPACEDISPLAY_DIRHELPER_H

#include <string>
#include <memory>

class FilePath;

class DirHelper {
public:
    /**
     * Creates directory at specified path. All other dirs/files
     * will be created in that directory
     * @param path
     */
    explicit DirHelper(const std::string &path);

    ~DirHelper();

    bool createDir(const std::string &path);

    bool createFile(const std::string &path);

    bool deleteDir(const std::string &path);

    bool rename(const std::string &oldPath, const std::string &newPath);

private:
    std::unique_ptr<FilePath> root;
};

#endif //SPACEDISPLAY_DIRHELPER_H
