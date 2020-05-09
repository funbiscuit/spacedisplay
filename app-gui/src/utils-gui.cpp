#include "utils-gui.h"

#include <portable-file-dialogs.h>



std::string UtilsGui::select_folder(const std::string& title)
{
    // simple wrapper so we need less memory to compile
    return pfd::select_folder(title).result();
}

void UtilsGui::message_box(const std::string& title, const std::string& text)
{
    pfd::message(title, text, pfd::choice::ok).result();
}

QColor UtilsGui::blend(const QColor& foreground, const QColor& background, qreal bg_amount)
{
    return QColor::fromRgbF(
            foreground.redF()*(1.0-bg_amount) + background.redF()*bg_amount,
            foreground.greenF()*(1.0-bg_amount) + background.greenF()*bg_amount,
            foreground.blueF()*(1.0-bg_amount) + background.blueF()*bg_amount);
}

bool UtilsGui::isDark(const QColor& bg)
{
    return bg.lightnessF() < 0.5;
}
