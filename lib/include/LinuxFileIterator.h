#ifndef SPACEDISPLAY_LINUXFILEITERATOR_H
#define SPACEDISPLAY_LINUXFILEITERATOR_H

#include "FileIterator.h"

#include <dirent.h>

class LinuxFileIterator : public FileIterator {
private:
    explicit LinuxFileIterator(std::string path);

public:
    ~LinuxFileIterator() override;

    void operator++() override;

    bool isValid() const override;

    const std::string &getName() const override;

    bool isDir() const override;

    int64_t getSize() const override;

    friend std::unique_ptr<FileIterator> FileIterator::create(const std::string &path);

private:
    bool valid;
    std::string name;
    bool dir;
    int64_t size;

    DIR *dirp;
    std::string path;

    /**
     * Gets file data for the next file returned by handle
     * If handle is new (just returned by opendir) then filedata for the first file will be read
     */
    void getNextFileData();
};

#endif //SPACEDISPLAY_LINUXFILEITERATOR_H
