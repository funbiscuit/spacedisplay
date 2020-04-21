#ifndef SPACEDISPLAY_SPACEVIEW_H
#define SPACEDISPLAY_SPACEVIEW_H

#include <QWidget>
#include <vector>
#include <string>
#include <memory>

class SpaceScanner;
class FileEntryPopup;
class MainWindow;
class ColorTheme;
class FilePath;
class FileViewDB;
class FileEntryView;


class SpaceView : public QWidget
{
Q_OBJECT
public:
    explicit SpaceView(MainWindow* parent);

    void setScanner(SpaceScanner* _scanner);
    void setTheme(std::shared_ptr<ColorTheme> theme);

    void onScanUpdate();

    bool isAtRoot();
    void rescanDir(const FilePath& dir_path);

    FilePath* getCurrentPath();

    FileEntryView* getHoveredEntry();

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

    uint64_t getDisplayedUsed();

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

    /**
     * History contains all paths we can get back to. historyPointer points to currentPath
     * but it is not safe to access history at pointer, it might be null (it is temporarily
     * moved to currentPath. And after navigating back or forward currentPath is moved back
     * to history and another path is moved to currentPath
     */
    std::vector<std::unique_ptr<FilePath>> pathHistory;
    size_t pathHistoryPointer=0;

    int currentDepth = 5;
    std::unique_ptr<FilePath> currentPath;
    std::unique_ptr<FileViewDB> viewDB;
    //TODO probably should store paths to entries instead of raw pointers
    FileEntryView* hoveredEntry=nullptr;
    FileEntryView* tooltipEntry = nullptr;
    SpaceScanner* scanner= nullptr;

    int textHeight = 0;
    bool showAvailable = false;
    bool showUnknown = true;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void historyPush();


    void allocateEntries();

    void cleanupEntryPointers();

    bool updateHoveredView(FileEntryView* prevHovered = nullptr);

    void drawView(QPainter& painter, const FileEntryView& file, int nestLevel, bool forceFill);
    void drawViewTitle(QPainter& painter, const QColor& bg, const FileEntryView& file);
    void drawViewText(QPainter& painter, const QColor& bg, const FileEntryView& file);
    bool drawViewBg(QPainter& painter, QColor& bg_out, const FileEntryView& file, bool fillDir);

};

#endif //SPACEDISPLAY_SPACEVIEW_H
