#ifndef SPACEDISPLAY_FILETOOLTIP_H
#define SPACEDISPLAY_FILETOOLTIP_H

#include <string>
#include <chrono>

class FileTooltip
{
public:
    FileTooltip() = default;
    ~FileTooltip();

    /**
     * Set delay for showing tooltip.
     * @param ms
     */
    void setDelay(int ms);

    /**
     * Set tooltip data.
     * If tooltip with the same text is already shown, nothing changes.
     * If tooltip with the same text is about to be shown, its coordinates will be updated.
     * If no tooltip is shown, this tooltip will be show after delay.
     * If tooltip with different text was shown before, it will be hidden immediately.
     * @param globalX
     * @param globalY
     * @param text_
     */
    void setTooltip(int globalX, int globalY, const std::string& text_);

    /**
     * Hide tooltip if any is shown
     */
    void hideTooltip();

    /**
     * Should be called from time to time so tooltip is shown after specified delay
     */
    void onTimer();

private:

    // time when request for showing tooltip is sent
    std::chrono::time_point<std::chrono::steady_clock> requestTimepoint;

    std::string text;
    int x = 0;
    int y = 0;

    int tooltipDelayMs = 0;

    bool tooltipPending = false;
    bool isShown = false;


    void showTooltip();

};


#endif //SPACEDISPLAY_FILETOOLTIP_H
