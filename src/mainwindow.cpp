
#include <iostream>
#include <QtWidgets>
#include <portable-file-dialogs.h>

#include "mainwindow.h"
#include "spaceview.h"
#include "spacescanner.h"


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
        : spaceWidget(new SpaceView)
{
    scanner = std::make_unique<SpaceScanner>();

    setCentralWidget(spaceWidget);

    createActions();
    createStatusBar();

    readSettings();


    setUnifiedTitleAndToolBarOnMac(true);

    setWindowTitle("Space Display");

    timerId = startTimer(12);

    setEnabledActions(ActionMask::NEW_SCAN);
}

MainWindow::~MainWindow()
{
    killTimer(timerId);
}

void MainWindow::onScanUpdate()
{
    spaceWidget->onScanUpdate();
    updateAvailableActions();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(scanner->is_running())
    {
        scanner->stop_scan();
        while(scanner->is_running())
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    //maybe not really needed since system will free all memory by itself
    scanner->reset_database();

    writeSettings();
    event->accept();
}

void MainWindow::newScan()
{
    auto parts = scanner->get_available_roots();
    std::cout << parts.size()<<"\n";
    if(parts.empty())
        return;


    QMenu menu(this);
    auto titleAction = menu.addAction("Choose scan target:");
    titleAction->setEnabled(false);
    for(const auto& part : parts)
    {
        auto action = new QAction(part.c_str(), this);
        connect(action, &QAction::triggered, this, [part, this]()
        {
            startScan(part);
        });
        menu.addAction(action);
    }
    auto browseAction = new QAction("Browse...", this);
    connect(browseAction, &QAction::triggered, this, [this]()
    {
        std::cout<<"browse\n";
        auto path = pfd::select_folder("Choose a directory to scan").result();
        if(!path.empty())
        {
            if(path.back() != '/' && path.back() != '\\')
                path.append("/");
            startScan(path);
        }
    });
    menu.addAction(browseAction);

    menu.exec(QCursor::pos()+QPoint(10,10));
}

void MainWindow::startScan(const std::string& path)
{
    if(scanner->is_running())
    {
        scanner->stop_scan();
        //todo move to scanner function
        while(scanner->is_running())
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

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
    scanner->scan_dir(path);
    spaceWidget->setScanner(scanner.get());
    onScanUpdate();
}

void MainWindow::goBack()
{
    spaceWidget->navigateBack();
    updateAvailableActions();
}
void MainWindow::goForward()
{
    spaceWidget->navigateForward();
    updateAvailableActions();
}
void MainWindow::goUp()
{
    if(!spaceWidget->navigateUp())
        goHome();
    updateAvailableActions();
}
void MainWindow::goHome()
{
    spaceWidget->setScanner(scanner.get());
    updateAvailableActions();
}
void MainWindow::refreshView()
{
    disableActions(ActionMask::REFRESH);

    if(spaceWidget->isAtRoot())
    {
        std::string path = scanner->get_root_path();
        spaceWidget->rescanDir(path);
        spaceWidget->clearHistory();

        goHome();
    } else
    {
        auto path = spaceWidget->getCurrentPath();
        spaceWidget->rescanDir(path);
    }
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
}
void MainWindow::toggleUnknown()
{
    spaceWidget->setShowUnknownSpace(toggleUnknownAct->isChecked());
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

    if(scanner->can_refresh())
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

//    const QIcon newIcon = QIcon::fromTheme("document-new", QIcon(":/images/new.png"));
//    QAction *newAct = new QAction(newIcon, tr("&New"), this);
    newAct = std::make_unique<QAction>("&New Scan", this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip("Start a new scan");
    connect(newAct.get(), &QAction::triggered, this, &MainWindow::newScan);

    backAct = std::make_unique<QAction>("Go &Back", this);
    backAct->setShortcuts(QKeySequence::Back);
    backAct->setStatusTip("Go to previous view");
    connect(backAct.get(), &QAction::triggered, this, &MainWindow::goBack);

    forwardAct = std::make_unique<QAction>("Go &Forward", this);
    forwardAct->setShortcuts(QKeySequence::Forward);
    forwardAct->setStatusTip("Go to next view");
    connect(forwardAct.get(), &QAction::triggered, this, &MainWindow::goForward);

    upAct = std::make_unique<QAction>("Go &Up", this);
    upAct->setStatusTip("Go to parent view");
    connect(upAct.get(), &QAction::triggered, this, &MainWindow::goUp);

    homeAct = std::make_unique<QAction>("Go &Home", this);
    homeAct->setStatusTip("Go to home (root) view");
    connect(homeAct.get(), &QAction::triggered, this, &MainWindow::goHome);

    rescanAct = std::make_unique<QAction>("&Rescan", this);
    rescanAct->setShortcuts(QKeySequence::Refresh);
    rescanAct->setStatusTip("Rescan current view");
    connect(rescanAct.get(), &QAction::triggered, this, &MainWindow::refreshView);

    lessDetailAct = std::make_unique<QAction>("Less details", this);
    lessDetailAct->setShortcuts(QKeySequence::ZoomOut);
    lessDetailAct->setStatusTip("Show less details");
    connect(lessDetailAct.get(), &QAction::triggered, this, &MainWindow::lessDetail);

    moreDetailAct = std::make_unique<QAction>("More details", this);
    moreDetailAct->setShortcuts(QKeySequence::ZoomIn);
    moreDetailAct->setStatusTip("Show more details");
    connect(moreDetailAct.get(), &QAction::triggered, this, &MainWindow::moreDetail);

    toggleFreeAct = std::make_unique<QAction>("Free space", this);
    toggleFreeAct->setStatusTip("Show/Hide free space");
    toggleFreeAct->setCheckable(true);
    connect(toggleFreeAct.get(), &QAction::triggered, this, &MainWindow::toggleFree);

    toggleUnknownAct = std::make_unique<QAction>("Unknown space", this);
//    toggleUnknownAct = new QAction("Unknown space", this);
    toggleUnknownAct->setStatusTip("Show/Hide unknown/unscanned space");
    toggleUnknownAct->setCheckable(true);
    connect(toggleUnknownAct.get(), &QAction::triggered, this, &MainWindow::toggleUnknown);

    //create menubar

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
            spaceWidget->increaseDetail();
            event->accept();
            return;
        }
        else if(event->delta()<0 && spaceWidget->canDecreaseDetail())
        {
            spaceWidget->decreaseDetail();
            event->accept();
            return;
        }
    }

    QWidget::wheelEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
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
    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
    if (geometry.isEmpty())
    {
        const QRect availableGeometry = QApplication::desktop()->availableGeometry(this);
        resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
        move((availableGeometry.width() - width()) / 2,
             (availableGeometry.height() - height()) / 2);
    } else
        restoreGeometry(geometry);
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    bool canRefresh = scanner->can_refresh();
    static bool prevCanRefresh = canRefresh;
    event->accept();
    if(scanner->has_changes())
        onScanUpdate();
    if(canRefresh != prevCanRefresh)
    {
        prevCanRefresh = canRefresh;
        updateAvailableActions();
    }
}
