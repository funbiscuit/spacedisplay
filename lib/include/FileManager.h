#ifndef SPACEDISPLAY_FILEMANAGER_H
#define SPACEDISPLAY_FILEMANAGER_H

#include <string>

namespace FileManager {
    /**
     * Opens folder in default file manager.
     * All slashes are converted to platform specific slashes
     * On Windows uses explorer
     * On Linux uses xdg-open to run default file manager
     * @param path - path to folder
     */
    void openFolder(const std::string &path);

    /**
     * Selects file in default file manager.
     * All slashes are converted to platform specific slashes
     * If there is slash at the end, it will be removed
     * On Windows uses explorer
     * On Linux tries to detect default file manager first.
     * Can detect Dolphin, Nautilus and Nemo and uses appropriate command to select file.
     * Thunar doesn't seem to support selecting files.
     * If file manager was not detected, uses xdg-open to open parent directory (file itself is not selected)
     * @param path - path to file
     */
    void showFile(const std::string &path);
}

#endif //SPACEDISPLAY_FILEMANAGER_H
