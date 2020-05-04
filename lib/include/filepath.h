#ifndef SPACEDISPLAY_FILEPATH_H
#define SPACEDISPLAY_FILEPATH_H

#include <string>
#include <vector>

class FilePath
{
public:
    enum class CompareResult {
        PARENT,
        CHILD,
        DIFFERENT,
        EQUAL
    };

    /**
     * Constructs a path object given a root string
     * Path can be constructed only from root directory (slash at the end is optional).
     * Then other directories can be appended to it.
     * At the end file can be appended (then nothing else could be appended, until file is popped)
     * String will be normalized (all slashes replaced with platform specific slashes)
     * @param root - root for constructed path (e.g. `D:\`, `/` or `/home/user`)
     * @param crc - crc of given string
     * @throws std::invalid_argument if construction failed
     */
    explicit FilePath(const std::string& root, uint16_t crc = 0);

    /**
     * Constructs new path by given full path and root
     * Root should be prefix of path
     * Path and root should be non empty
     * If path ends with slash, it is considered directory, otherwise it is a file.
     * Root may end with slash, but it is not required.
     * @param path - full path (e.g. /home/usr/test)
     * @param root - root for path (e.g. /home/)
     * @throws std::invalid_argument if construction failed
     */
    explicit FilePath(const std::string& path, const std::string& root);
    
    /**
     * Returns correct path to this file or directory
     * Uses '/' on linux and '\' on windows
     * @param addDirSlash if true and this path is path to dir, than a closing slash will be added
     * @return
     */
    std::string getPath(bool addDirSlash = true) const;
    
    std::string getRoot() const;

    const std::vector<std::string>& getParts() const;

    /**
     * Return crc16 this path
     * Crc is calculated for each part of this path and then xor'ed together
     * @return
     */
    uint16_t getPathCrc() const;

    /**
     * Returns name of file or directory this path is pointing to.
     * If already at root, the whole root would be returned.
     * Root returned "AS IS", otherwise slash at the end is removed.
     * @return
     */
    std::string getName() const;
    
    /**
     * Adds directory to the end of the path. If path already ends with file, returns false
     * @param name of the directory
     * @param crc - crc of given string
     * @return true on success, false otherwise
     * @throws std::invalid_argument if name is empty
     */
    bool addDir(const std::string& name, uint16_t crc = 0);
    
    /**
     * Adds file to the end of the path. If path already ends with file, exception is thrown.
     * After file is added, nothing else can be added.
     * @param name of the directory
     * @param crc - crc of given string
     * @throws std::invalid_argument if name is empty
     */
    bool addFile(const std::string& name, uint16_t crc = 0);
    
    /**
     * Checks if current path is pointing to directory or not
     * If path is invalid, returns false
     * @return true if directory, false otherwise
     */
    bool isDir() const;
    
    /**
     * Checks if it is possible to navigate up
     * @return false if path is already at root, true otherwise
     */
    bool canGoUp() const;
    
    /**
     * If it is possible to navigate up, will change path to its parent directory.
     * @return false if path is already at root, true otherwise
     */
    bool goUp();

    /**
     * Compares this path to specified path
     * @param path
     * @return
     */
    CompareResult compareTo(const FilePath& path);

    /**
     * Converts this path to make it relative to provided path
     * Provided path should be parent of this path
     * If path can't be converted, false is returned
     * @param path
     * @return
     */
    bool makeRelativeTo(const FilePath& parentPath);

private:
    
    /**
     * Contains parts of current path. Each part contains at most one slash - at the end.
     * Root part can have multiple slashes (e.g. /home/user/Desktop/)
     * Each part has slash at the end if it is a directory and doesn't have a slash if it is a file.
     * Only last part can be a file.
     * On Windows first part is X:\ where X is disk letter or some othe path (e.g. C:\Windows\System32\)
     * On Linux first part could be '/' or '/usr/lib/' or anything else of such format
     */
    std::vector<std::string> parts;

    std::vector<uint16_t> pathCrcs;
    
    /**
     * Converts all slashes to platform correct ones, removes unnecessary slashes.
     * @param path - path to normalize (will be edited in place)
     * @param addTrailingSlash - if true and there is no slash at the end - slash will be added
     *                           if false and there is a slash at the end - it will be removed
     */
    static void normalize(std::string& path, bool addTrailingSlash);
};


#endif //SPACEDISPLAY_FILEPATH_H
