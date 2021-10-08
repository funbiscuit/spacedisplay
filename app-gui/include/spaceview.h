#ifndef SPACEDISPLAY_SPACEVIEW_H
#define SPACEDISPLAY_SPACEVIEW_H

#include <QWidget>
#include <vector>
#include <string>
#include <memory>
#include <functional>

#include "customtheme.h"

class SpaceScanner;

class FileEntryPopup;

class FilePath;

class FileViewDB;

class FileEntryView;

class FileTooltip;


class SpaceView : public QWidget {
Q_OBJECT
public:
    SpaceView();

    ~SpaceView() override;

    void setScanner(std::unique_ptr<SpaceScanner> _scanner);

    void setCustomPalette(const CustomPalette &palette);

    bool getWatcherLimits(int64_t &watchedNow, int64_t &watchLimit);

    void onScanUpdate();

    bool canRefresh();

    bool isAtRoot();

    void rescanDir(const FilePath &dir_path);

    void rescanCurrentView();

    bool isScanOpen();

    int getScanProgress();

    /**
     * Returns true if it possible to determine scan progress.
     * For example, if we scan partition and can get total occupied space.
     * @return
     */
    bool isProgressKnown();

    bool getSpace(uint64_t &scannedVisible, uint64_t &scannedHidden, uint64_t &available, uint64_t &total);

    int64_t getScannedFiles();

    int64_t getScannedDirs();

    /**
     * Sets callback that will be called when any action occurs.
     * For example, user navigates to different directory or presses rescan dir
     * menu item.
     * @param callback
     * @return
     */
    void setOnActionCallback(std::function<void(void)> callback);

    /**
     * Sets callback that will be called when watch limit is exceeded.
     * This happens on linux when there is not big enough user limit for inotify watches
     * @param callback
     * @return
     */
    void setOnWatchLimitCallback(std::function<void(void)> callback);

    /**
     * Sets callback that will be called when this view requests new scan dialog
     * This happens when user opens context menu in empty space
     * menu item.
     * @param callback
     * @return
     */
    void setOnNewScanRequestCallback(std::function<void(void)> callback);

    /**
     * Navigates to the parent directory
     * @return whether after navigating up we're still not at the root directory
     * so if true - we can navigate up more
     */
    bool navigateUp();

    /**
     * Navigates to the root directory
     */
    void navigateHome();

    /**
     * Navigates to the previous view
     */
    void navigateBack();

    /**
     * Navigates to the next view
     */
    void navigateForward();

    void increaseDetail();

    void decreaseDetail();


    /**
     * @return whether we still have something in history
     * so if true - we can navigate back
     */
    bool canNavigateBack();

    /**
     * @return whether we still have something in history (in future)
     * so if true - we can navigate forward
     */
    bool canNavigateForward();

    /**
     * @return whether we can increase detalization (show deeper)
     */
    bool canIncreaseDetail();

    /**
     * @return whether we can decrease detalization (show less deep)
     */
    bool canDecreaseDetail();

    bool canTogglePause();

    bool isPaused();

    void setPause(bool paused);

    void setShowFreeSpace(bool showFree);

    void setShowUnknownSpace(bool showUnknown);

    void clearHistory();

protected:
    const int MIN_DEPTH = 1;
    const int MAX_DEPTH = 9;

    CustomPalette customPalette;

    QPixmap bgIcon;

    int mouseX = -1;
    int mouseY = -1;

    std::unique_ptr<FileEntryPopup> entryPopup;
    std::unique_ptr<FileTooltip> fileTooltip;

    /**
     * History contains all paths we can get back to. historyPointer points to currentPath
     * but it is not safe to access history at pointer, it might be null (it is temporarily
     * moved to currentPath. And after navigating back or forward currentPath is moved back
     * to history and another path is moved to currentPath
     */
    std::vector<std::unique_ptr<FilePath>> pathHistory;
    size_t pathHistoryPointer = 0;

    int currentDepth = 5;
    std::unique_ptr<FilePath> currentPath;
    std::unique_ptr<FileViewDB> viewDB;
    std::unique_ptr<FilePath> hoveredPath;
    std::unique_ptr<FilePath> currentScannedPath;
    uint64_t hoveredId = 0;
    uint64_t currentScannedId = 0;
    std::unique_ptr<SpaceScanner> scanner;

    /**
     * Id of timer that is used to check whether new information is available
     * in scanner
     */
    int scanTimerId;

    int textHeight = 0;
    bool showAvailable = false;
    bool showUnknown = true;

    std::function<void(void)> onActionCallback;
    std::function<void(void)> onNewScanRequestCallback;
    std::function<void(void)> onWatchLimitCallback;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void leaveEvent(QEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

    void timerEvent(QTimerEvent *event) override;

    void historyPush();

    void allocateEntries();

    bool updateHoveredView();

    void updateScannedView();

    void drawView(QPainter &painter, const FileEntryView &file, int nestLevel, bool forceFill);

    void drawViewTitle(QPainter &painter, const QColor &bg, const FileEntryView &file);

    void drawViewText(QPainter &painter, const QColor &bg, const FileEntryView &file);

    bool drawViewBg(QPainter &painter, QColor &bg_out, const FileEntryView &file, bool fillDir);

};

#endif //SPACEDISPLAY_SPACEVIEW_H
