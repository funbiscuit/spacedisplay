#ifndef SPACEDISPLAY_FILEITERATOR_H
#define SPACEDISPLAY_FILEITERATOR_H

#include <string>
#include <memory>
#include <stdexcept>

class FileIterator {
protected:
    FileIterator() = default;

public:
    virtual ~FileIterator() = default;

    /**
     * Create platform specific iterator at provided path
     * @param path
     * @return pointer to created iterator
     */
    static std::unique_ptr<FileIterator> create(const std::string &path);

    // can't copy
    FileIterator(const FileIterator &) = delete;

    FileIterator &operator=(const FileIterator &) = delete;

    /**
     * Moves iterator to next file/dir
     * @throws std:out_of_range if iterator was not valid
     */
    virtual void operator++() = 0;

    /**
     * Check if iterator is valid, otherwise its members are not valid
     */
    virtual bool isValid() const = 0;

    /**
     * @return name of current directory/file or empty if not valid
     * @throws std:out_of_range if iterator was not valid
     */
    virtual const std::string &getName() const = 0;

    /**
     * @return true if currently pointing to directory, false otherwise
     * @throws std:out_of_range if iterator was not valid
     */
    virtual bool isDir() const = 0;

    /**
     * @return size of current file, 0 otherwise
     * @throws std:out_of_range if iterator was not valid
     */
    virtual int64_t getSize() const = 0;

    /**
     * Assert that this iterator is valid, otherwise throw an error
     * @throws std::out_of_range if iterator not valid
     */
    void assertValid() const {
        if (!isValid())
            throw std::out_of_range("Iterator not valid");
    }
};

#endif //SPACEDISPLAY_FILEITERATOR_H
