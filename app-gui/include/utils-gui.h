#ifndef SPACEDISPLAY_UTILS_GUI_H
#define SPACEDISPLAY_UTILS_GUI_H

#include <string>

#include <QColor>

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

    /**
     * Blend fg color with bg color by bg_amount
     * @param fg
     * @param bg
     * @param bg_amount
     * @return fg*(1-bg_amount)+bg*bg_amount
     */
    QColor blend(const QColor& fg, const QColor& bg, qreal bg_amount);
}

#endif //SPACEDISPLAY_UTILS_GUI_H
