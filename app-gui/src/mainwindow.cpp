
#include <iostream>
#include <QtWidgets>

#include "mainwindow.h"
#include "inotifydialog.h"
#include "logdialog.h"
#include "spaceview.h"
#include "statusview.h"
#include "spacescanner.h"
#include "resources.h"
#include "customtheme.h"
#include "utils.h"
#include "utils-gui.h"
#include "platformutils.h"
#include "logger.h"


MainWindow::ActionMask operator|(MainWindow::ActionMask lhs, MainWindow::ActionMask rhs) {
    return static_cast<MainWindow::ActionMask> (
            static_cast<std::underlying_type<MainWindow::ActionMask>::type>(lhs) |
            static_cast<std::underlying_type<MainWindow::ActionMask>::type>(rhs)
    );
}

MainWindow::ActionMask operator&(MainWindow::ActionMask lhs, MainWindow::ActionMask rhs) {
    return static_cast<MainWindow::ActionMask> (
            static_cast<std::underlying_type<MainWindow::ActionMask>::type>(lhs) &
            static_cast<std::underlying_type<MainWindow::ActionMask>::type>(rhs)
    );
}

MainWindow::ActionMask operator~(MainWindow::ActionMask mask) {
    return static_cast<MainWindow::ActionMask> (
            ~static_cast<std::underlying_type<MainWindow::ActionMask>::type>(mask)
    );
}

bool operator!(MainWindow::ActionMask mask) {
    return !static_cast<bool>(mask);
}

MainWindow::MainWindow()
        : spaceWidget(new SpaceView), statusView(new StatusView) {
    setMinimumSize(600, 400);
    layout = Utils::make_unique<QVBoxLayout>();
    logger = std::make_shared<Logger>();
    logWindow = Utils::make_unique<LogDialog>(logger);

    statusView->setMode(StatusView::Mode::NO_SCAN);

    auto window = new QWidget();

    window->setLayout(layout.get());

    setCentralWidget(window);

    layout->addWidget(spaceWidget.get(), 1);
    layout->addWidget(statusView, 0);

    spaceWidget->setOnActionCallback([this]() {
        onScanUpdate();
    });
    spaceWidget->setOnNewScanRequestCallback([this]() {
        newScan();
    });
    spaceWidget->setOnWatchLimitCallback([this]() {
        watchLimitExceeded = true;
    });

    logWindow->setOnDataChanged([this](bool hasNew) {
        if (logWindow->isHidden())
            updateLogIcon(hasNew);
    });

    createActions();
//    createStatusBar();

    readSettings();


    setUnifiedTitleAndToolBarOnMac(true);

    setWindowTitle("Space Display");

    setEnabledActions(ActionMask::NEW_SCAN);
}

MainWindow::~MainWindow() {
    layout->removeWidget(spaceWidget.get());
}

void MainWindow::onScanUpdate() {
    updateStatusView();
    updateAvailableActions();
    if (spaceWidget->getScanProgress() == 100 && watchLimitExceeded && !watchLimitReported) {
        int64_t watchedNow, watchLimit;
        spaceWidget->getWatcherLimits(watchedNow, watchLimit);
        int64_t dirCount = spaceWidget->getScannedDirs();
        auto newLimit = watchLimit + dirCount - watchedNow + 2048; // add some extra
        watchLimitReported = true;
        watchLimitExceeded = false;
        //TODO add option to hide these messages
        InotifyDialog dlg(watchLimit, newLimit, logger.get());
        dlg.exec();
    }
}

void MainWindow::updateStatusView() {
    statusView->setMode(StatusView::Mode::NO_SCAN);
    if (!spaceWidget->isScanOpen()) {
        //if nothing opened, we don't have any data so just call repaint
        statusView->repaint();
        return;
    }

    uint64_t scannedVisible, scannedHidden, available, total;

    if (!spaceWidget->getSpace(scannedVisible, scannedHidden, available, total))
        return; //should not happen

    auto unknown = total - available - scannedVisible - scannedHidden;

    auto progress = spaceWidget->getScanProgress();
    if (progress == 100)
        statusView->setMode(StatusView::Mode::SCAN_FINISHED);
    else if (spaceWidget->isProgressKnown())
        statusView->setMode(StatusView::Mode::SCANNING_DEFINITE);
    else
        statusView->setMode(StatusView::Mode::SCANNING_INDEFINITE);

    if (spaceWidget->isAtRoot())
        statusView->setSpaceHighlight(toggleFreeAct->isChecked(), toggleUnknownAct->isChecked());
    else
        statusView->setSpaceHighlight(false, false);

    // if exact progress not known, we should minimize available and unknown bars
    // and maximize what is actually scanned
    statusView->setMaximizeSpace(!spaceWidget->isProgressKnown());

    statusView->setScannedFiles(spaceWidget->getScannedFiles());
    statusView->setProgress(progress);
    statusView->setSpace((float) scannedVisible, (float) scannedHidden,
                         (float) available, (float) unknown);
    statusView->repaint();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    writeSettings();
    logWindow->close();
    event->accept();
}

void MainWindow::newScan() {
    std::vector<std::string> scanTargets;
    scanTargets = PlatformUtils::getAvailableMounts();
    if (scanTargets.empty())
        return;


    QMenu menu(this);
    auto titleAction = menu.addAction("Choose scan target:");
    titleAction->setEnabled(false);
    menu.addSeparator();
    for (const auto &target : scanTargets) {
        auto action = new QAction(target.c_str(), this);
        connect(action, &QAction::triggered, this, [target, this]() {
            startScan(target);
        });
        menu.addAction(action);
    }
    auto browseAction = new QAction("Browse...", this);
    connect(browseAction, &QAction::triggered, this, [this]() {
        auto path = UtilsGui::select_folder("Choose a directory to scan");
        if (!path.empty()) {
            if (path.back() != PlatformUtils::filePathSeparator)
                path.push_back(PlatformUtils::filePathSeparator);
            startScan(path);
        }
    });
    menu.addAction(browseAction);

    menu.exec(QCursor::pos() + QPoint(10, 10));
}

void MainWindow::startScan(const std::string &path) {
    auto parts = PlatformUtils::getAvailableMounts();
    isRootScanned = false;
    for (const auto &part : parts) {
        if (part == path) {
            isRootScanned = true;
            break;
        }
    }

    std::cout << "Scan: " << path << "\n";
    spaceWidget->clearHistory();
    setEnabledActions(ActionMask::NEW_SCAN);
    if (isRootScanned)
        enableActions(ActionMask::TOGGLE_FREE | ActionMask::TOGGLE_UNKNOWN);
    else
        disableActions(ActionMask::TOGGLE_FREE | ActionMask::TOGGLE_UNKNOWN);

    toggleUnknownAct->setChecked(isRootScanned);
    toggleFreeAct->setChecked(false);
    spaceWidget->setShowUnknownSpace(isRootScanned);
    spaceWidget->setShowFreeSpace(false);

    try {
        auto scanner = Utils::make_unique<SpaceScanner>(path);
        scanner->setLogger(logger);
        spaceWidget->setScanner(std::move(scanner));
        watchLimitReported = false;
        onScanUpdate();
    } catch (std::runtime_error &) {
        spaceWidget->setScanner(nullptr);
        onScanUpdate();
        UtilsGui::message_box("Can't open path for scanning:", path);
    }
}

void MainWindow::goBack() {
    spaceWidget->navigateBack();
    onScanUpdate();
}

void MainWindow::goForward() {
    spaceWidget->navigateForward();
    onScanUpdate();
}

void MainWindow::goUp() {
    spaceWidget->navigateUp();
    onScanUpdate();
}

void MainWindow::goHome() {
    spaceWidget->navigateHome();
    onScanUpdate();
}

void MainWindow::togglePause() {
    spaceWidget->setPause(pauseAct->isChecked());
    onScanUpdate();
}

void MainWindow::refreshView() {
    disableActions(ActionMask::REFRESH);
    spaceWidget->rescanCurrentView();
    onScanUpdate();
}

void MainWindow::lessDetail() {
    spaceWidget->decreaseDetail();
    updateAvailableActions();
}

void MainWindow::moreDetail() {
    spaceWidget->increaseDetail();
    updateAvailableActions();
}

void MainWindow::toggleFree() {
    spaceWidget->setShowFreeSpace(toggleFreeAct->isChecked());
    updateStatusView();
}

void MainWindow::toggleUnknown() {
    spaceWidget->setShowUnknownSpace(toggleUnknownAct->isChecked());
    updateStatusView();
}

void MainWindow::switchTheme() {
    setTheme(!customPalette.isDark(), true);
    QSettings settings;
    settings.setValue(SETTINGS_THEME, customPalette.isDark());
}

void MainWindow::showLog() {
    logWindow->show();
    updateLogIcon(false);
}

void MainWindow::setTheme(bool isDark, bool updateIcons_) {
    customPalette.setTheme(isDark ? CustomPalette::Theme::DARK : CustomPalette::Theme::LIGHT);

    QApplication::setPalette(customPalette.getPalette());
    QToolTip::setPalette(palette());
    spaceWidget->setCustomPalette(customPalette);
    statusView->setCustomPalette(customPalette);

    if (updateIcons_)
        updateIcons();
}

void MainWindow::updateIcons() {
    using namespace ResourceBuilder;
    std::vector<std::pair<ResId, QAction *>> iconPairs = {
            std::make_pair(ResId::__ICONS_SVG_NEW_SCAN_SVG, newAct.get()),
            std::make_pair(ResId::__ICONS_SVG_PAUSE_SVG, pauseAct.get()),
            std::make_pair(ResId::__ICONS_SVG_REFRESH_SVG, rescanAct.get()),
            std::make_pair(ResId::__ICONS_SVG_ARROW_BACK_SVG, backAct.get()),
            std::make_pair(ResId::__ICONS_SVG_ARROW_FORWARD_SVG, forwardAct.get()),
            std::make_pair(ResId::__ICONS_SVG_FOLDER_NAVIGATE_UP_SVG, upAct.get()),
            std::make_pair(ResId::__ICONS_SVG_HOME_SVG, homeAct.get()),
            std::make_pair(ResId::__ICONS_SVG_ZOOM_OUT_SVG, lessDetailAct.get()),
            std::make_pair(ResId::__ICONS_SVG_ZOOM_IN_SVG, moreDetailAct.get()),
            std::make_pair(ResId::__ICONS_SVG_SPACE_FREE_SVG, toggleFreeAct.get()),
            std::make_pair(ResId::__ICONS_SVG_SPACE_UNKNOWN_SVG, toggleUnknownAct.get()),
            std::make_pair(ResId::__ICONS_SVG_SMOOTH_MODE_SVG, themeAct.get()),
    };
    for (auto &pair : iconPairs) {
        if (pair.second) {
            auto icon = customPalette.createIcon(pair.first);
            //FIXME calling QAction::setIcon() seems to not releasing previous icon which leads to memory leak
            pair.second->setIcon(icon);
        }
    }
    if (logAct->property(LOG_ICON_STATE).toBool())
        logAct->setIcon(customPalette.createIcon(ResId::__ICONS_SVG_NOTIFY_NEW_SVG));
    else
        logAct->setIcon(customPalette.createIcon(ResId::__ICONS_SVG_NOTIFY_SVG));
}

void MainWindow::updateLogIcon(bool hasNew) {
    if (!logAct)
        return;
    using namespace ResourceBuilder;
    auto v = logAct->property(LOG_ICON_STATE);
    if (v.toBool() == hasNew)
        return;

    if (hasNew)
        logAct->setIcon(customPalette.createIcon(ResId::__ICONS_SVG_NOTIFY_NEW_SVG));
    else
        logAct->setIcon(customPalette.createIcon(ResId::__ICONS_SVG_NOTIFY_SVG));
    logAct->setProperty(LOG_ICON_STATE, hasNew);
}

void MainWindow::about() {
    QMessageBox::about(this, tr("About Space Display"),
                       tr("<b>Space Display</b> will help you to scan "
                          "your storage and determine what files use space"));
}


void MainWindow::updateAvailableActions() {
    if (spaceWidget->canNavigateBack())
        enableActions(ActionMask::BACK);
    else
        disableActions(ActionMask::BACK);

    if (spaceWidget->canNavigateForward())
        enableActions(ActionMask::FORWARD);
    else
        disableActions(ActionMask::FORWARD);


    if (spaceWidget->canIncreaseDetail())
        enableActions(ActionMask::MORE_DETAIL);
    else
        disableActions(ActionMask::MORE_DETAIL);

    if (spaceWidget->canDecreaseDetail())
        enableActions(ActionMask::LESS_DETAIL);
    else
        disableActions(ActionMask::LESS_DETAIL);

    if (spaceWidget->canTogglePause())
        enableActions(ActionMask::PAUSE);
    else
        disableActions(ActionMask::PAUSE);

    pauseAct->setChecked(spaceWidget->isPaused());

    if (spaceWidget->canRefresh())
        enableActions(ActionMask::REFRESH);
    else
        disableActions(ActionMask::REFRESH);

    if (spaceWidget->isAtRoot()) {
        disableActions(ActionMask::HOME | ActionMask::NAVIGATE_UP);
        if (isRootScanned)
            enableActions(ActionMask::TOGGLE_FREE | ActionMask::TOGGLE_UNKNOWN);
        else
            disableActions(ActionMask::TOGGLE_FREE | ActionMask::TOGGLE_UNKNOWN);

    } else {
        enableActions(ActionMask::HOME | ActionMask::NAVIGATE_UP);

        disableActions(ActionMask::TOGGLE_FREE | ActionMask::TOGGLE_UNKNOWN);
    }
}

void MainWindow::setEnabledActions(ActionMask actions) {
    enabledActions = actions;
    newAct->setEnabled(!!(enabledActions & ActionMask::NEW_SCAN));
    backAct->setEnabled(!!(enabledActions & ActionMask::BACK));
    forwardAct->setEnabled(!!(enabledActions & ActionMask::FORWARD));
    upAct->setEnabled(!!(enabledActions & ActionMask::NAVIGATE_UP));
    homeAct->setEnabled(!!(enabledActions & ActionMask::HOME));
    pauseAct->setEnabled(!!(enabledActions & ActionMask::PAUSE));
    rescanAct->setEnabled(!!(enabledActions & ActionMask::REFRESH));
    lessDetailAct->setEnabled(!!(enabledActions & ActionMask::LESS_DETAIL));
    moreDetailAct->setEnabled(!!(enabledActions & ActionMask::MORE_DETAIL));
    toggleFreeAct->setEnabled(!!(enabledActions & ActionMask::TOGGLE_FREE));
    toggleUnknownAct->setEnabled(!!(enabledActions & ActionMask::TOGGLE_UNKNOWN));
}

void MainWindow::enableActions(ActionMask actions) {
    setEnabledActions(enabledActions | actions);
}

void MainWindow::disableActions(ActionMask actions) {
    setEnabledActions(enabledActions & (~actions));
}

void MainWindow::createActions() {
    //create all actions first

    auto exitAct = new QAction("E&xit", this);
    connect(exitAct, &QAction::triggered, this, &MainWindow::close);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip("Exit the application");

    auto aboutAct = new QAction("&About", this);
    aboutAct->setStatusTip("Show the application's About box");
    connect(aboutAct, &QAction::triggered, this, &MainWindow::about);

    newAct = Utils::make_unique<QAction>("&New Scan", this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip("Start a new scan");
    connect(newAct.get(), &QAction::triggered, this, &MainWindow::newScan);

    backAct = Utils::make_unique<QAction>("Go &Back", this);
    backAct->setShortcuts(QKeySequence::Back);
    backAct->setStatusTip("Go to previous view");
    connect(backAct.get(), &QAction::triggered, this, &MainWindow::goBack);

    forwardAct = Utils::make_unique<QAction>("Go &Forward", this);
    forwardAct->setShortcuts(QKeySequence::Forward);
    forwardAct->setStatusTip("Go to next view");
    connect(forwardAct.get(), &QAction::triggered, this, &MainWindow::goForward);

    upAct = Utils::make_unique<QAction>("Go &Up", this);
    upAct->setStatusTip("Go to parent view");
    connect(upAct.get(), &QAction::triggered, this, &MainWindow::goUp);

    homeAct = Utils::make_unique<QAction>("Go &Home", this);
    homeAct->setStatusTip("Go to home (root) view");
    connect(homeAct.get(), &QAction::triggered, this, &MainWindow::goHome);

    pauseAct = Utils::make_unique<QAction>("&Pause", this);
    pauseAct->setStatusTip("Pause current scan");
    pauseAct->setCheckable(true);
    connect(pauseAct.get(), &QAction::triggered, this, &MainWindow::togglePause);

    rescanAct = Utils::make_unique<QAction>("&Rescan", this);
    rescanAct->setShortcuts(QKeySequence::Refresh);
    rescanAct->setStatusTip("Rescan current view");
    connect(rescanAct.get(), &QAction::triggered, this, &MainWindow::refreshView);

    lessDetailAct = Utils::make_unique<QAction>("Less details", this);
    lessDetailAct->setShortcuts(QKeySequence::ZoomOut);
    lessDetailAct->setStatusTip("Show less details");
    connect(lessDetailAct.get(), &QAction::triggered, this, &MainWindow::lessDetail);

    moreDetailAct = Utils::make_unique<QAction>("More details", this);
    moreDetailAct->setShortcuts(QKeySequence::ZoomIn);
    moreDetailAct->setStatusTip("Show more details");
    connect(moreDetailAct.get(), &QAction::triggered, this, &MainWindow::moreDetail);

    toggleFreeAct = Utils::make_unique<QAction>("Free space", this);
    toggleFreeAct->setStatusTip("Show/Hide free space");
    toggleFreeAct->setCheckable(true);
    connect(toggleFreeAct.get(), &QAction::triggered, this, &MainWindow::toggleFree);

    toggleUnknownAct = Utils::make_unique<QAction>("Unknown space", this);
    toggleUnknownAct->setStatusTip("Show/Hide unknown/unscanned space");
    toggleUnknownAct->setCheckable(true);
    connect(toggleUnknownAct.get(), &QAction::triggered, this, &MainWindow::toggleUnknown);

    themeAct = Utils::make_unique<QAction>("Switch theme", this);
    connect(themeAct.get(), &QAction::triggered, this, &MainWindow::switchTheme);

    logAct = Utils::make_unique<QAction>("Show log", this);
    connect(logAct.get(), &QAction::triggered, this, &MainWindow::showLog);

    //create menubar
/*
    auto fileMenu = menuBar()->addMenu("&Scan");
    auto goMenu = menuBar()->addMenu("&Go");
    auto viewMenu = menuBar()->addMenu("&View");
    auto helpMenu = menuBar()->addMenu(tr("&Help"));

    //create file menu
    fileMenu->addAction(newAct.get());
    fileMenu->addAction(rescanAct.get());
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    //create navigate menu
    goMenu->addAction(backAct.get());
    goMenu->addAction(forwardAct.get());
    goMenu->addAction(upAct.get());
    goMenu->addAction(homeAct.get());

    //create view menu
    viewMenu->addAction(lessDetailAct.get());
    viewMenu->addAction(moreDetailAct.get());
    viewMenu->addAction(toggleFreeAct.get());
    viewMenu->addAction(toggleUnknownAct.get());

    //create help menu
    helpMenu->addAction(aboutAct);
*/

    //create toolbar
    QToolBar *mainToolbar = addToolBar("Toolbar");
    mainToolbar->setMovable(false);

    mainToolbar->addAction(newAct.get());
    mainToolbar->addAction(pauseAct.get());
    mainToolbar->addAction(rescanAct.get());
    mainToolbar->addSeparator();

    mainToolbar->addAction(backAct.get());
    mainToolbar->addAction(forwardAct.get());
    mainToolbar->addAction(upAct.get());
    mainToolbar->addAction(homeAct.get());
    mainToolbar->addSeparator();

    mainToolbar->addAction(lessDetailAct.get());
    mainToolbar->addAction(moreDetailAct.get());
    mainToolbar->addAction(toggleFreeAct.get());
    mainToolbar->addAction(toggleUnknownAct.get());
    mainToolbar->addSeparator();

    mainToolbar->addAction(themeAct.get());
    mainToolbar->addAction(logAct.get());

    updateIcons();
//    cutAct->setEnabled(false);
//    connect(textEdit, &QPlainTextEdit::copyAvailable, cutAct, &QAction::setEnabled);
}

void MainWindow::createStatusBar() {
    statusBar()->showMessage("");
}

void MainWindow::wheelEvent(QWheelEvent *event) {
    bool isWidgetHovered = spaceWidget->rect().contains(spaceWidget->mapFromGlobal(event->globalPos()));
    if (isWidgetHovered) {
        if (event->delta() > 0 && spaceWidget->canIncreaseDetail()) {
            moreDetail();
            event->accept();
            return;
        } else if (event->delta() < 0 && spaceWidget->canDecreaseDetail()) {
            lessDetail();
            event->accept();
            return;
        }
    }

    QWidget::wheelEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    switch (event->button()) {
        case Qt::BackButton:
            goBack();
            break;
        case Qt::ForwardButton:
            goForward();
            break;
        default:
            break;
    }
    if (event->button() == Qt::RightButton) {
//        bool isWidgetHovered = spaceWidget->rect().contains(spaceWidget->mapFromGlobal(event->globalPos()));
//        if(isWidgetHovered)
//        {
//            if(!scanner->is_loaded())
//            {
//                newScan();
//                event->accept();
//                return;
//            }
//        }
    }
    QWidget::mouseReleaseEvent(event);
}

void MainWindow::readSettings() {
    QSettings settings;
    const QByteArray geometry = settings.value(SETTINGS_GEOMETRY, QByteArray()).toByteArray();
    if (geometry.isEmpty()) {
        const QRect availableGeometry = QApplication::desktop()->availableGeometry(this);
        resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
        move((availableGeometry.width() - width()) / 2,
             (availableGeometry.height() - height()) / 2);
    } else
        restoreGeometry(geometry);

    setTheme(settings.value(SETTINGS_THEME, false).toBool(), true);
}

void MainWindow::writeSettings() {
    QSettings settings;
    settings.setValue(SETTINGS_GEOMETRY, saveGeometry());
    settings.setValue(SETTINGS_THEME, customPalette.isDark());
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    QWidget::mouseMoveEvent(event);
}
