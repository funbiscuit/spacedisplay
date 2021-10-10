#ifndef SPACEDISPLAY_PLATFORM_UTILS_H
#define SPACEDISPLAY_PLATFORM_UTILS_H

#include <string>
#include <vector>
#include <memory>

/**
 * Collection of functions that a platform dependent
 */
namespace PlatformUtils {
    /**
     * Check if provided path exists and can be opened for scan
     * @param path to check
     * @return true on success, false otherwise
     */
    bool can_scan_dir(const std::string &path);

    /**
     * Scan filesystem for available mount points (that can be scanned)
     * For example, on windows it will usually include "C:\"
     * On linux it will usually include "/"
     * @return vector containing paths of mount points that can be scanned
     */
    std::vector<std::string> getAvailableMounts();

    /**
     * Scan filesystem for available excluded paths,
     * that should not be scanned (important for *nix since
     * we can't scan such paths as /proc or /sys)
     * On Windows it is empty
     * @return vector containing paths of excluded paths
     */
    std::vector<std::string> getExcludedPaths();

    /**
     * Gets total and available space of filesystem mounted at specified path
     * @param path to mount point or any of its sub directories
     * @param totalSpace - total space in bytes that this filesystem has
     * @param availableSpace - available (free) space in bytes that this filesystem has
     * @return true if spaces are read successfully, false otherwise
     */
    bool get_mount_space(const std::string &path, uint64_t &totalSpace, uint64_t &availableSpace);

    /**
     * Opens folder in default file manager.
     * Path should have correct slashes for this platform.
     * On Windows uses explorer
     * On Linux uses xdg-open to run default file manager
     * @param folder_path
     */
    void open_folder_in_file_manager(const char *folder_path);

    /**
     * Selects file in default file manager.
     * Path should have correct slashes for this platform.
     * On Windows uses explorer
     * On Linux tries to detect default file manager first.
     * Can detect Dolphin, Nautilus and Nemo and uses appropriate command to select file.
     * Thunar doesn't seem to support selecting files.
     * If file manager was not detected, uses xdg-open to open parent directory (file itself is not selected)
     * @param file_path
     */
    void show_file_in_file_manager(const char *file_path);

    /**
     * Deletes directory and all files inside
     * Provided path should be global path, otherwise it is not thread-safe to call this.
     * Since working directory could change while function is executing.
     * @param path
     * @return true if was deleted, false otherwise
     */
    bool deleteDir(const std::string &path);

    /**
     * filePathSeparator - file path separator that is native for the platform
     * invertedFilePathSeparator - file path separator that is not native for the platform
     */
#ifdef _WIN32
    const char filePathSeparator = '\\';
    const char invertedFilePathSeparator = '/';
#else
    const char filePathSeparator = '/';
    const char invertedFilePathSeparator = '\\';
#endif

    /**
     * Functions specific to certain platform
     */
#ifdef _WIN32

    /**
     * Convert utf8 encoded string to widechar encoded wstring
     * @param str - string in utf8 encoding
     * @return wstring in widechar encoding
     */
    std::wstring str2wstr(std::string const &str);

    /**
     * Convert widechar wstring to utf8 encoded string
     * @param wstr - wstring in widechar encoding
     * @return string in utf8 encoding
     */
    std::string wstr2str(std::wstring const &wstr);


    /**
     * Convert short path (utf-8 encoded) to long path (also utf-8).
     * Conversion is done "in place". If conversion failed, path is not changed
     * @param shortPath
     * @return true on success
     */
    bool toLongPath(std::string &shortPath);

#endif

};

#endif //SPACEDISPLAY_PLATFORM_UTILS_H
