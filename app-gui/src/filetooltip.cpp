
#include "filetooltip.h"
#include <qtooltip.h>

FileTooltip::~FileTooltip()
{
    hideTooltip();
}

void FileTooltip::setDelay(int ms)
{
    tooltipDelayMs = ms;
}

void FileTooltip::setTooltip(int globalX, int globalY, const std::string &text_)
{
    // new tooltip is shown only if text is different
    //TODO probably should check also other things
    if(text != text_)
    {
        hideTooltip();
        requestTimepoint = std::chrono::steady_clock::now();
        tooltipPending = true;
        text = text_;
    }
    x = globalX;
    y = globalY;

}

void FileTooltip::hideTooltip()
{
    tooltipPending = false;
    if(isShown && QToolTip::isVisible())
    {
        QToolTip::hideText();
        isShown = false;
    }
}

void FileTooltip::onTimer()
{
    if(!tooltipPending)
        return;
    using namespace std::chrono;
    auto now = steady_clock::now();
    if(duration_cast<milliseconds>(now-requestTimepoint).count()>tooltipDelayMs)
    {
        // show tooltip after specified delay
        showTooltip();
    }
}

void FileTooltip::showTooltip()
{
    tooltipPending = false;
    isShown = true;
    QToolTip::showText(QPoint(x, y), text.c_str());
}
