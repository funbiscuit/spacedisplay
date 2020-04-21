#ifndef SPACEDISPLAY_UTILS_GUI_H
#define SPACEDISPLAY_UTILS_GUI_H

#include <string>

namespace UtilsGui {

    /**
     * Let's user select desired folder with their default manager and returns its path
     * @param title of window for selecting folder
     * @return path to selected folder or empty string if cancelled
     */
    std::string select_folder(const std::string& title);

    /**
     * Show's user some message text
     * @param title of message box
     * @param text in message box
     */
    void message_box(const std::string& title, const std::string& text);
}

#endif //SPACEDISPLAY_UTILS_GUI_H
