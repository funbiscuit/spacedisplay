#ifndef SPACEDISPLAY_FILEPATH_H
#define SPACEDISPLAY_FILEPATH_H

#include <string>
#include <vector>

class FilePath
{
public:
    /**
     * Constructs a path object given a root string
     * Path can be constructed only from root directory (slash at the end is optional).
     * Then other directories can be appended to it.
     * At the end file can be appended (then nothing else could be appended, until file is popped)
     * String will be normalized (all slashes replaced with platform specific slashes)
     * @param root - root for constructed path (e.g. `D:\`, `/` or `/home/user`)
     */
    explicit FilePath(const std::string& root);
    
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
     * Returns name of file or directory this path is pointing to.
     * If already at root, the whole root would be returned.
     * Root returned "AS IS", otherwise slash at the end is removed.
     * @return
     */
    std::string getName() const;
    
    /**
     * Adds directory to the end of the path. If path already ends with file, returns false
     * @param name of the directory
     * @return true on success, false otherwise
     */
    bool addDir(const std::string& name);
    
    /**
     * Adds file to the end of the path. If path already ends with file, exception is thrown.
     * After file is added, nothing else can be added.
     * @param name of the directory
     */
    bool addFile(const std::string& name);
    
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
     * Sets new root for the path. All previous information is deleted.
     * @param root - new root
     */
    void setRoot(const std::string& root_);

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

    
    /**
     * Converts all slashes to platform correct ones, removes unnecessary slashes.
     * @param path - path to normalize (will be edited in place)
     * @param addTrailingSlash - if true and there is no slash at the end - slash will be added
     *                           if false and there is a slash at the end - it will be removed
     */
    static void normalize(std::string& path, bool addTrailingSlash);
};


#endif //SPACEDISPLAY_FILEPATH_H
