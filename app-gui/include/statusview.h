
#ifndef SPACEDISPLAY_STATUSBAR_WIDGET_H
#define SPACEDISPLAY_STATUSBAR_WIDGET_H

#include <QWidget>
#include <QPainter>

#include <string>
#include <memory>

#include "customtheme.h"

struct StatusPart {
    QColor color;
    bool isHidden = false;
    float weight = 0.f;
    std::string label;
    int textMargin = 10;
};

class StatusView : public QWidget {
Q_OBJECT
public:
    // depending on what mode is selected, status will display differently
    enum class Mode {
        NO_SCAN,
        SCANNING_DEFINITE,   //for scanning with known progress
        SCANNING_INDEFINITE, //for scanning with unknown progress
        SCAN_FINISHED
    };

    void setSpace(float scannedVisible, float scannedHidden, float available, float unknown);

    /**
     * Set percent of scan progress (from 0 to 100)
     * But 100% will never be shown. If scan in progress, it will be limited to 99%
     * @param progress
     */
    void setProgress(int progress);

    void setCustomPalette(const CustomPalette &palette);

    void setMode(Mode mode);

    /**
     * Set what space to highlight, otherwise it will be drawn but not highlighted
     * @param available
     * @param unknown
     */
    void setSpaceHighlight(bool available, bool unknown);

    /**
     * If true, unknown and available spaces will be minimized
     * All remaining space will be filled by scanned space (it will be maximized)
     * @param minimize
     */
    void setMaximizeSpace(bool maximize);

    /**
     * Set how much files are scanned
     * @param files
     */
    void setScannedFiles(int64_t files);

    QSize minimumSizeHint() const override;

protected:
    float scannedSpaceVisible = 0.f;
    float scannedSpaceHidden = 0.f;
    float availableSpace = 0.f;
    float unknownSpace = 1.f;

    int scanProgress = 0;

    int64_t scannedFiles = 0;

    Mode currentMode;

    bool highlightAvailable = false;
    bool highlightUnknown = false;
    bool maximizeSpace = false;

    const int textPadding = 3;

    CustomPalette customPalette;

    std::vector<StatusPart> parts;

    std::string getScanStatusText();

    int getStatusWidth(const QFontMetrics &fm);

    std::string getScannedFilesText();

    void allocateParts(const QFontMetrics &fm, float width);

    void paintEvent(QPaintEvent *event) override;
};

#endif //SPACEDISPLAY_STATUSBAR_WIDGET_H
