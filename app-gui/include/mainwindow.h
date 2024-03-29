
#ifndef SPACEDISPLAY_MAINWINDOW_H
#define SPACEDISPLAY_MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <memory>

#include "customtheme.h"


QT_BEGIN_NAMESPACE
class QAction;

class QMenu;

QT_END_NAMESPACE

class SpaceView;

class StatusView;

class SpaceScanner;

class Logger;

class LogDialog;


class MainWindow : public QMainWindow {
Q_OBJECT

public:
    MainWindow();

    ~MainWindow() override;

    enum class ActionMask : uint32_t {
        NEW_SCAN = 1U << 0U,
        BACK = 1U << 1U,
        FORWARD = 1U << 2U,
        NAVIGATE_UP = 1U << 3U,
        HOME = 1U << 4U,
        PAUSE = 1U << 5U,
        REFRESH = 1U << 6U,
        LESS_DETAIL = 1U << 7U,
        MORE_DETAIL = 1U << 8U,
        TOGGLE_FREE = 1U << 9U,
        TOGGLE_UNKNOWN = 1U << 10U,
    };

    void updateAvailableActions();

    void updateStatusView();

protected:
    void closeEvent(QCloseEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

public slots:

    void newScan();

    void goBack();

    void goForward();

    void goUp();

    void goHome();

    void togglePause();

    void refreshView();

    void lessDetail();

    void moreDetail();

    void toggleFree();

    void toggleUnknown();

    void switchTheme();

    void about();

    void showLog();

private:
    void createActions();

    void createStatusBar();

    void readSettings();

    void writeSettings();

    void setTheme(bool isDark, bool updateIcons);

    void onScanUpdate();

    void startScan(const std::string &path);


    void setEnabledActions(ActionMask actions);

    void enableActions(ActionMask actions);

    void disableActions(ActionMask actions);

    void updateIcons();

    void updateLogIcon(bool hasNew);

    //settings keys
    const char *SETTINGS_GEOMETRY = "geometry";
    const char *SETTINGS_THEME = "dark_theme";

    const char *LOG_ICON_STATE = "log_icon";

    //actions
    std::unique_ptr<QAction> newAct;
    std::unique_ptr<QAction> backAct;
    std::unique_ptr<QAction> forwardAct;
    std::unique_ptr<QAction> upAct;
    std::unique_ptr<QAction> homeAct;
    std::unique_ptr<QAction> pauseAct;
    std::unique_ptr<QAction> rescanAct;
    std::unique_ptr<QAction> lessDetailAct;
    std::unique_ptr<QAction> moreDetailAct;
    std::unique_ptr<QAction> toggleFreeAct;
    std::unique_ptr<QAction> toggleUnknownAct;
    std::unique_ptr<QAction> themeAct;
    std::unique_ptr<QAction> logAct;


    ActionMask enabledActions;
    bool isRootScanned = false;

    bool watchLimitReported = false;
    bool watchLimitExceeded = false;

    std::unique_ptr<QVBoxLayout> layout;
    CustomPalette customPalette;

    std::unique_ptr<SpaceView> spaceWidget;
    std::unique_ptr<LogDialog> logWindow;
    std::shared_ptr<Logger> logger;
    StatusView *statusView;
};

#endif //SPACEDISPLAY_MAINWINDOW_H
