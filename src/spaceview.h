#ifndef SPACEDISPLAY_SPACEVIEW_H
#define SPACEDISPLAY_SPACEVIEW_H

#include <QWidget>
#include <vector>
#include <string>
#include <memory>
#include "fileentryshared.h"

class SpaceScanner;
class FileEntryPopup;
class MainWindow;
class ColorTheme;


class SpaceView : public QWidget
{
Q_OBJECT
public:
    explicit SpaceView(MainWindow* parent);

    void setScanner(SpaceScanner* _scanner);
    void setTheme(std::shared_ptr<ColorTheme> theme);

    void onScanUpdate();

    bool isAtRoot();
    void rescanDir(const std::string &dir_path);

    std::string getCurrentPath()
    {
        return currentPath;
    }

    FileEntryShared* getHoveredEntry();

    /**
     * Navigates to the parent directory
     * @return whether after navigating up we're still not at the root directory
     * so if true - we can navigate up more
     */
    bool navigateUp();

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

    uint64_t getHiddenSize();

    void setShowFreeSpace(bool showFree);
    void setShowUnknownSpace(bool showUnknown);

    void clearHistory();

protected:
    MainWindow* parent;
    const int MIN_DEPTH =1;
    const int MAX_DEPTH =9;
    std::shared_ptr<ColorTheme> colorTheme;

    QPixmap bgIcon;

    int mouseX=-1;
    int mouseY=-1;

    std::unique_ptr<FileEntryPopup> entryPopup;

    std::vector<std::string> pathHistory;
    size_t pathHistoryPointer=0;

    int currentDepth = 5;
    std::string currentPath;
    FileEntrySharedPtr root=nullptr;
    FileEntryShared* hoveredEntry=nullptr;
    FileEntryShared* tooltipEntry = nullptr;
    SpaceScanner* scanner= nullptr;

    int textHeight = 0;
    uint16_t fileEntryShowFlags = FileEntryShared::INCLUDE_UNKNOWN_SPACE;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void historyPush();


    void allocateEntries();

    bool updateHoveredView(FileEntryShared* prevHovered = nullptr);

    void drawView(QPainter& painter, const FileEntrySharedPtr &file, int nestLevel, bool forceFill);
    void drawViewTitle(QPainter& painter, const QColor& bg, const FileEntrySharedPtr &file);
    void drawViewText(QPainter& painter, const QColor& bg, const FileEntrySharedPtr &file);
    bool drawViewBg(QPainter& painter, QColor& bg_out, const FileEntrySharedPtr &file, bool fillDir);

};

#endif //SPACEDISPLAY_SPACEVIEW_H
