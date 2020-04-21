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
