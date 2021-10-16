#ifndef SPACEDISPLAY_WINFILEITERATOR_H
#define SPACEDISPLAY_WINFILEITERATOR_H

#include "FileIterator.h"

#include <Windows.h>

class WinFileIterator : public FileIterator {
private:
    explicit WinFileIterator(const std::string &path);

public:
    ~WinFileIterator() override;

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

    HANDLE dirHandle;

    /**
     * Gets file data for the first or next file returned by handle
     */
    void processFileData(bool isFirst, WIN32_FIND_DATAW *fileData);
};

#endif //SPACEDISPLAY_WINFILEITERATOR_H
