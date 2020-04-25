
#include <iostream>
#include <QtWidgets>

#include "mainwindow.h"
#include "spaceview.h"
#include "statusview.h"
#include "spacescanner.h"
#include "resources.h"
#include "colortheme.h"
#include "utils.h"
#include "utils-gui.h"
#include "platformutils.h"


MainWindow::ActionMask operator|(MainWindow::ActionMask lhs, MainWindow::ActionMask rhs)
{
    return static_cast<MainWindow::ActionMask> (
            static_cast<std::underlying_type<MainWindow::ActionMask>::type>(lhs) |
            static_cast<std::underlying_type<MainWindow::ActionMask>::type>(rhs)
    );
}
MainWindow::ActionMask operator&(MainWindow::ActionMask lhs, MainWindow::ActionMask rhs)
{
    return static_cast<MainWindow::ActionMask> (
            static_cast<std::underlying_type<MainWindow::ActionMask>::type>(lhs) &
            static_cast<std::underlying_type<MainWindow::ActionMask>::type>(rhs)
    );
}

MainWindow::ActionMask operator~(MainWindow::ActionMask mask)
{
    return static_cast<MainWindow::ActionMask> (
            ~static_cast<std::underlying_type<MainWindow::ActionMask>::type>(mask)
    );
}

bool operator!(MainWindow::ActionMask mask)
{
    return !static_cast<bool>(mask);
}

MainWindow::MainWindow()
        : spaceWidget(new SpaceView), statusView(new StatusView)
{
    auto p = palette();
    //at start create theme with default colors and then switch to appropriate custom theme
    colorTheme = std::make_shared<ColorTheme>(p.window().color(),
            p.windowText().color(), ColorTheme::NativeStyle::NATIVE);
    setTheme(colorTheme->isDark(), false);

    setMinimumSize(600, 400);
    scanner = Utils::make_unique<SpaceScanner>();
    layout = Utils::make_unique<QVBoxLayout>();
//    layout->setContentsMargins(0,0,0,0);
//    layout->setSpacing(0);

    statusView->setMode(StatusView::Mode::NO_SCAN);

    auto window = new QWidget();

    window->setLayout(layout.get());

    setCentralWidget(window);

    layout->addWidget(spaceWidget.get(),1);
    layout->addWidget(statusView,0);

    spaceWidget->setOnActionCallback([this](){
        onScanUpdate();
    });
    spaceWidget->setOnNewScanRequestCallback([this](){
        newScan();
    });

    createActions();
//    createStatusBar();

    readSettings();


    setUnifiedTitleAndToolBarOnMac(true);

    setWindowTitle("Space Display");

    setEnabledActions(ActionMask::NEW_SCAN);
}

MainWindow::~MainWindow()
{
    layout->removeWidget(spaceWidget.get());
}

void MainWindow::onScanUpdate()
{
    updateStatusView();
    updateAvailableActions();
}

void MainWindow::updateStatusView()
{
    statusView->setMode(StatusView::Mode::NO_SCAN);
    if(!spaceWidget->isScanOpen())
    {
        //if nothing opened, we don't have any data so just call repaint
        statusView->repaint();
        return;
    }

    uint64_t scannedVisible, scannedHidden, available, total;

    if(!spaceWidget->getSpace(scannedVisible, scannedHidden, available, total))
        return; //should not happen

    auto unknown= total - available - scannedVisible - scannedHidden;

    auto progress = spaceWidget->getScanProgress();
    if(progress == 100)
        statusView->setMode(StatusView::Mode::SCAN_FINISHED);
    else if(spaceWidget->isProgressKnown())
        statusView->setMode(StatusView::Mode::SCANNING_DEFINITE);
    else
        statusView->setMode(StatusView::Mode::SCANNING_INDEFINITE);

    if(spaceWidget->isAtRoot())
        statusView->setSpaceHighlight(toggleFreeAct->isChecked(), toggleUnknownAct->isChecked());
    else
        statusView->setSpaceHighlight(false, false);

    // if exact progress not known, we should minimize available and unknown bars
    // and maximize what is actually scanned
    statusView->setMaximizeSpace(!spaceWidget->isProgressKnown());

    statusView->setProgress(progress);
    statusView->setSpace((float) scannedVisible, (float) scannedHidden,
                         (float) available, (float) unknown);
    statusView->repaint();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}

void MainWindow::newScan()
{
    std::vector<std::string> scanTargets;
    Utils::getMountPoints(scanTargets);
    if(scanTargets.empty())
        return;


    QMenu menu(this);
    auto titleAction = menu.addAction("Choose scan target:");
    titleAction->setEnabled(false);
    for(const auto& target : scanTargets)
    {
        auto action = new QAction(target.c_str(), this);
        connect(action, &QAction::triggered, this, [target, this]()
        {
            startScan(target);
        });
        menu.addAction(action);
    }
    auto browseAction = new QAction("Browse...", this);
    connect(browseAction, &QAction::triggered, this, [this]()
    {
        auto path = UtilsGui::select_folder("Choose a directory to scan");
        if(!path.empty())
        {
            if(path.back() != PlatformUtils::filePathSeparator)
                path.push_back(PlatformUtils::filePathSeparator);
            startScan(path);
        }
    });
    menu.addAction(browseAction);

    menu.exec(QCursor::pos()+QPoint(10,10));
}

void MainWindow::startScan(const std::string& path)
{
    scanner->stop_scan();

    auto parts = scanner->get_available_roots();
    isRootScanned = false;
    for(const auto& part : parts)
    {
        if(part == path)
        {
            isRootScanned = true;
            break;
        }
    }

    std::cout << "Scan: "<<path<<"\n";
    spaceWidget->clearHistory();
    setEnabledActions(ActionMask::NEW_SCAN);
    if(isRootScanned)
        enableActions(ActionMask::TOGGLE_FREE | ActionMask::TOGGLE_UNKNOWN);
    else
        disableActions(ActionMask::TOGGLE_FREE | ActionMask::TOGGLE_UNKNOWN);

    toggleUnknownAct->setChecked(isRootScanned);
    toggleFreeAct->setChecked(false);
    spaceWidget->setShowUnknownSpace(isRootScanned);
    spaceWidget->setShowFreeSpace(false);
    if(scanner->scan_dir(path) == ScannerError::NONE)
    {
        spaceWidget->setScanner(scanner.get());
        onScanUpdate();
    } else
    {
        spaceWidget->setScanner(nullptr);
        onScanUpdate();
        UtilsGui::message_box("Can't open path for scanning:", path);
    }
}

void MainWindow::goBack()
{
    spaceWidget->navigateBack();
    onScanUpdate();
}
void MainWindow::goForward()
{
    spaceWidget->navigateForward();
    onScanUpdate();
}
void MainWindow::goUp()
{
    spaceWidget->navigateUp();
    onScanUpdate();
}
void MainWindow::goHome()
{
    spaceWidget->navigateHome();
    onScanUpdate();
}
void MainWindow::refreshView()
{
    disableActions(ActionMask::REFRESH);
    spaceWidget->rescanCurrentView();
    onScanUpdate();
}
void MainWindow::lessDetail()
{
    spaceWidget->decreaseDetail();
    updateAvailableActions();
}
void MainWindow::moreDetail()
{
    spaceWidget->increaseDetail();
    updateAvailableActions();
}
void MainWindow::toggleFree()
{
    spaceWidget->setShowFreeSpace(toggleFreeAct->isChecked());
    updateStatusView();
}
void MainWindow::toggleUnknown()
{
    spaceWidget->setShowUnknownSpace(toggleUnknownAct->isChecked());
    updateStatusView();
}
void MainWindow::switchTheme()
{
    setTheme(!colorTheme->isDark(), true);
    QSettings settings;
    settings.setValue(SETTINGS_THEME, colorTheme->isDark());
}

void MainWindow::setTheme(bool isDark, bool updateIcons)
{
    if(isDark)
        colorTheme = std::make_shared<ColorTheme>(ColorTheme::CustomStyle::DARK);
    else
        colorTheme = std::make_shared<ColorTheme>(ColorTheme::CustomStyle::LIGHT);
    auto p = palette();
    p.setColor(QPalette::Window, colorTheme->background);
    p.setColor(QPalette::WindowText, colorTheme->foreground);
    p.setColor(QPalette::Light, p.window().color());    //hide white line on top of toolbar in windows
    setPalette(p);
    spaceWidget->setTheme(colorTheme);
    statusView->setTheme(colorTheme);

    if(updateIcons)
    {
        using namespace ResourceBuilder;
        std::vector<std::pair<ResId, QAction*>> iconPairs={
                std::make_pair(ResId::__ICONS_SVG_NEW_SCAN_SVG,newAct.get()),
                std::make_pair(ResId::__ICONS_SVG_REFRESH_SVG,rescanAct.get()),
                std::make_pair(ResId::__ICONS_SVG_ARROW_BACK_SVG,backAct.get()),
                std::make_pair(ResId::__ICONS_SVG_ARROW_FORWARD_SVG,forwardAct.get()),
                std::make_pair(ResId::__ICONS_SVG_FOLDER_NAVIGATE_UP_SVG,upAct.get()),
                std::make_pair(ResId::__ICONS_SVG_HOME_SVG,homeAct.get()),
                std::make_pair(ResId::__ICONS_SVG_ZOOM_OUT_SVG,lessDetailAct.get()),
                std::make_pair(ResId::__ICONS_SVG_ZOOM_IN_SVG,moreDetailAct.get()),
                std::make_pair(ResId::__ICONS_SVG_SPACE_FREE_SVG,toggleFreeAct.get()),
                std::make_pair(ResId::__ICONS_SVG_SPACE_UNKNOWN_SVG,toggleUnknownAct.get()),
                std::make_pair(ResId::__ICONS_SVG_SMOOTH_MODE_SVG,themeAct.get()),
        };
        for(auto& pair : iconPairs)
        {
            auto icon = colorTheme->createIcon(pair.first);
            //FIXME calling QAction::setIcon() seems to not releasing previous icon which leads to memory leak
            pair.second->setIcon(icon);
        }
    }

}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Space Display"),
                       tr("<b>Space Display</b> will help you to scan "
                          "your storage and determine what files use space"));
}


void MainWindow::updateAvailableActions()
{
    if(spaceWidget->canNavigateBack())
        enableActions(ActionMask::BACK);
    else
        disableActions(ActionMask::BACK);

    if(spaceWidget->canNavigateForward())
        enableActions(ActionMask::FORWARD);
    else
        disableActions(ActionMask::FORWARD);


    if(spaceWidget->canIncreaseDetail())
        enableActions(ActionMask::MORE_DETAIL);
    else
        disableActions(ActionMask::MORE_DETAIL);

    if(spaceWidget->canDecreaseDetail())
        enableActions(ActionMask::LESS_DETAIL);
    else
        disableActions(ActionMask::LESS_DETAIL);

    if(spaceWidget->canRefresh())
        enableActions(ActionMask::REFRESH);
    else
        disableActions(ActionMask::REFRESH);

    if(spaceWidget->isAtRoot())
    {
        disableActions(ActionMask::HOME | ActionMask::NAVIGATE_UP);
        if(isRootScanned)
            enableActions(ActionMask::TOGGLE_FREE | ActionMask::TOGGLE_UNKNOWN);
        else
            disableActions(ActionMask::TOGGLE_FREE | ActionMask::TOGGLE_UNKNOWN);

    }
    else
    {
        enableActions(ActionMask::HOME | ActionMask::NAVIGATE_UP);

        disableActions(ActionMask::TOGGLE_FREE | ActionMask::TOGGLE_UNKNOWN);
    }
}

void MainWindow::setEnabledActions(ActionMask actions)
{
    enabledActions=actions;
    newAct->setEnabled(!!(enabledActions & ActionMask::NEW_SCAN));
    backAct->setEnabled(!!(enabledActions & ActionMask::BACK));
    forwardAct->setEnabled(!!(enabledActions & ActionMask::FORWARD));
    upAct->setEnabled(!!(enabledActions & ActionMask::NAVIGATE_UP));
    homeAct->setEnabled(!!(enabledActions & ActionMask::HOME));
    rescanAct->setEnabled(!!(enabledActions & ActionMask::REFRESH));
    lessDetailAct->setEnabled(!!(enabledActions & ActionMask::LESS_DETAIL));
    moreDetailAct->setEnabled(!!(enabledActions & ActionMask::MORE_DETAIL));
    toggleFreeAct->setEnabled(!!(enabledActions & ActionMask::TOGGLE_FREE));
    toggleUnknownAct->setEnabled(!!(enabledActions & ActionMask::TOGGLE_UNKNOWN));
}

void MainWindow::enableActions(ActionMask actions)
{
    setEnabledActions(enabledActions | actions);
}

void MainWindow::disableActions(ActionMask actions)
{
    setEnabledActions(enabledActions & (~actions));
}

void MainWindow::createActions()
{
    //create all actions first

    auto exitAct = new QAction("E&xit", this);
    connect(exitAct, &QAction::triggered, this, &MainWindow::close);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip("Exit the application");

    auto aboutAct = new QAction("&About", this);
    aboutAct->setStatusTip("Show the application's About box");
    connect(aboutAct, &QAction::triggered, this, &MainWindow::about);

    const QIcon newIcon = colorTheme->createIcon(ResourceBuilder::ResId::__ICONS_SVG_NEW_SCAN_SVG);
    const QIcon rescanIcon = colorTheme->createIcon(ResourceBuilder::ResId::__ICONS_SVG_REFRESH_SVG);
    const QIcon backIcon = colorTheme->createIcon(ResourceBuilder::ResId::__ICONS_SVG_ARROW_BACK_SVG);
    const QIcon forwardIcon = colorTheme->createIcon(ResourceBuilder::ResId::__ICONS_SVG_ARROW_FORWARD_SVG);
    const QIcon upIcon = colorTheme->createIcon(ResourceBuilder::ResId::__ICONS_SVG_FOLDER_NAVIGATE_UP_SVG);
    const QIcon homeIcon = colorTheme->createIcon(ResourceBuilder::ResId::__ICONS_SVG_HOME_SVG);
    const QIcon lessIcon = colorTheme->createIcon(ResourceBuilder::ResId::__ICONS_SVG_ZOOM_OUT_SVG);
    const QIcon moreIcon = colorTheme->createIcon(ResourceBuilder::ResId::__ICONS_SVG_ZOOM_IN_SVG);
    const QIcon freeIcon = colorTheme->createIcon(ResourceBuilder::ResId::__ICONS_SVG_SPACE_FREE_SVG);
    const QIcon unknownIcon = colorTheme->createIcon(ResourceBuilder::ResId::__ICONS_SVG_SPACE_UNKNOWN_SVG);
    const QIcon themeIcon = colorTheme->createIcon(ResourceBuilder::ResId::__ICONS_SVG_SMOOTH_MODE_SVG);

    newAct = Utils::make_unique<QAction>(newIcon, "&New Scan", this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip("Start a new scan");
    connect(newAct.get(), &QAction::triggered, this, &MainWindow::newScan);

    backAct = Utils::make_unique<QAction>(backIcon,"Go &Back", this);
    backAct->setShortcuts(QKeySequence::Back);
    backAct->setStatusTip("Go to previous view");
    connect(backAct.get(), &QAction::triggered, this, &MainWindow::goBack);

    forwardAct = Utils::make_unique<QAction>(forwardIcon,"Go &Forward", this);
    forwardAct->setShortcuts(QKeySequence::Forward);
    forwardAct->setStatusTip("Go to next view");
    connect(forwardAct.get(), &QAction::triggered, this, &MainWindow::goForward);

    upAct = Utils::make_unique<QAction>(upIcon,"Go &Up", this);
    upAct->setStatusTip("Go to parent view");
    connect(upAct.get(), &QAction::triggered, this, &MainWindow::goUp);

    homeAct = Utils::make_unique<QAction>(homeIcon,"Go &Home", this);
    homeAct->setStatusTip("Go to home (root) view");
    connect(homeAct.get(), &QAction::triggered, this, &MainWindow::goHome);

    rescanAct = Utils::make_unique<QAction>(rescanIcon, "&Rescan", this);
    rescanAct->setShortcuts(QKeySequence::Refresh);
    rescanAct->setStatusTip("Rescan current view");
    connect(rescanAct.get(), &QAction::triggered, this, &MainWindow::refreshView);

    lessDetailAct = Utils::make_unique<QAction>(lessIcon, "Less details", this);
    lessDetailAct->setShortcuts(QKeySequence::ZoomOut);
    lessDetailAct->setStatusTip("Show less details");
    connect(lessDetailAct.get(), &QAction::triggered, this, &MainWindow::lessDetail);

    moreDetailAct = Utils::make_unique<QAction>(moreIcon, "More details", this);
    moreDetailAct->setShortcuts(QKeySequence::ZoomIn);
    moreDetailAct->setStatusTip("Show more details");
    connect(moreDetailAct.get(), &QAction::triggered, this, &MainWindow::moreDetail);

    toggleFreeAct = Utils::make_unique<QAction>(freeIcon, "Free space", this);
    toggleFreeAct->setStatusTip("Show/Hide free space");
    toggleFreeAct->setCheckable(true);
    connect(toggleFreeAct.get(), &QAction::triggered, this, &MainWindow::toggleFree);

    toggleUnknownAct = Utils::make_unique<QAction>(unknownIcon, "Unknown space", this);
    toggleUnknownAct->setStatusTip("Show/Hide unknown/unscanned space");
    toggleUnknownAct->setCheckable(true);
    connect(toggleUnknownAct.get(), &QAction::triggered, this, &MainWindow::toggleUnknown);

    themeAct = Utils::make_unique<QAction>(themeIcon, "Switch theme", this);
    connect(themeAct.get(), &QAction::triggered, this, &MainWindow::switchTheme);

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


//    cutAct->setEnabled(false);
//    connect(textEdit, &QPlainTextEdit::copyAvailable, cutAct, &QAction::setEnabled);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage("");
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    bool isWidgetHovered = spaceWidget->rect().contains(spaceWidget->mapFromGlobal(event->globalPos()));
    if(isWidgetHovered)
    {
        if(event->delta()>0 && spaceWidget->canIncreaseDetail())
        {
            moreDetail();
            event->accept();
            return;
        }
        else if(event->delta()<0 && spaceWidget->canDecreaseDetail())
        {
            lessDetail();
            event->accept();
            return;
        }
    }

    QWidget::wheelEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    switch (event->button())
    {
        case Qt::BackButton:
            goBack();
            break;
        case Qt::ForwardButton:
            goForward();
            break;
        default:
            break;
    }
    if(event->button() == Qt::RightButton)
    {
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

void MainWindow::readSettings()
{
    QSettings settings;
    const QByteArray geometry = settings.value(SETTINGS_GEOMETRY, QByteArray()).toByteArray();
    if (geometry.isEmpty())
    {
        const QRect availableGeometry = QApplication::desktop()->availableGeometry(this);
        resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
        move((availableGeometry.width() - width()) / 2,
             (availableGeometry.height() - height()) / 2);
    } else
        restoreGeometry(geometry);

    if(settings.contains(SETTINGS_THEME))
    {
        bool isDark = settings.value(SETTINGS_THEME).toBool();
        if(colorTheme->isDark() != isDark)
            switchTheme();
    }
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue(SETTINGS_GEOMETRY, saveGeometry());
    settings.setValue(SETTINGS_THEME, colorTheme->isDark());
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
}
