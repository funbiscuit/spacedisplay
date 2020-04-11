#ifndef SPACEDISPLAY_PLATFORM_UTILS_H
#define SPACEDISPLAY_PLATFORM_UTILS_H

#include <string>
#include <vector>

/**
 * Collection of functions that a platform dependent
 */
namespace PlatformUtils
{
    /**
     * Check if provided path exists and can be opened for scan
     * @param path to check
     * @return true on success, false otherwise
     */
    bool can_scan_dir(const std::string& path);

    /**
     * Scan filesystem for available mount points (that can be scanned)
     * and excluded mount points, that should not be scanned (important for *nix since
     * we can't scan such mounts as /proc or /sys
     * @param availableMounts vector containing paths of safe mount points
     * @param excludedMounts  vector containing paths of mount points that should not be scanned
     */
    void get_mount_points(std::vector<std::string>& availableMounts, std::vector<std::string>& excludedMounts);

    /**
     * Gets total and available space of filesystem mounted at specified path
     * @param path to mount point or any of its sub directories
     * @param totalSpace - total space in bytes that this filesystem has
     * @param availableSpace - available (free) space in bytes that this filesystem has
     * @return true if spaces are read successfully, false otherwise
     */
    bool get_mount_space(const std::string& path, uint64_t& totalSpace, uint64_t& availableSpace);

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

#endif

};

#endif //SPACEDISPLAY_PLATFORM_UTILS_H
