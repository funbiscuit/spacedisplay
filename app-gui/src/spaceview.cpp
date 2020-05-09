
#include <QtWidgets>
#include <iostream>
#include "spaceview.h"
#include "spacescanner.h"
#include "fileentrypopup.h"
#include "filepath.h"
#include "fileviewdb.h"
#include "fileentryview.h"
#include "filetooltip.h"
#include "resources.h"

SpaceView::SpaceView() :
        QWidget(), onActionCallback(nullptr)
{
    viewDB = Utils::make_unique<FileViewDB>();

    setMouseTracking(true);
    entryPopup = Utils::make_unique<FileEntryPopup>(this);
    fileTooltip = Utils::make_unique<FileTooltip>();
    fileTooltip->setDelay(250);

    entryPopup->onRescanListener = [this](const FilePath& path)
    {
        rescanDir(path);
        if(onActionCallback)
            onActionCallback();
    };

    auto& r = Resources::get();
    bgIcon = r.get_vector_pixmap(
            ResourceBuilder::ResId::__ICONS_SVG_APPICON_BW_SVG,
            256);


    scanTimerId = startTimer(12);
}

SpaceView::~SpaceView()
{
    killTimer(scanTimerId);
}

void SpaceView::timerEvent(QTimerEvent *event)
{
    event->accept();
    fileTooltip->onTimer();
    if(!scanner)
        return;

    if(scanner->has_changes())
    {
        onScanUpdate();
        if(onActionCallback)
            onActionCallback();
    }
}

void SpaceView::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    int width = size().width();
    int height = size().height();

    if(viewDB && viewDB->isReady())
    {
        textHeight = painter.fontMetrics().height();
        viewDB->processEntry([this, &painter] (const FileEntryView& root){
            drawView(painter, root, currentDepth, true);
        });
    } else {
        int x0=(width-bgIcon.width())/2;
        int y0=(height-bgIcon.height())/2;
        painter.drawPixmap(x0,y0, bgIcon);
        auto col = palette().window().color();
        col.setAlpha(150);
        painter.fillRect(x0, y0, bgIcon.width(), bgIcon.height(), col);
    }
}

void SpaceView::drawView(QPainter& painter, const FileEntryView& file, int nestLevel, bool forceFill)
{
    QColor bg;
    if(!drawViewBg(painter, bg, file, forceFill || file.get_type() != FileEntryView::EntryType::DIRECTORY))
        return;


    if(nestLevel>0 && file.get_type() == FileEntryView::EntryType::DIRECTORY && !file.get_children().empty())
    {
        drawViewTitle(painter, bg, file);
        for(const auto& child : file.get_children())
        {
            //if we just drawn hovered or scanned view, we need to draw backgrounds for all its children
            drawView(painter, *child, nestLevel - 1,
                    file.getId() == hoveredId || file.getId() == currentScannedId);
        }
    } else
    {
        drawViewText(painter, bg, file);
    }
}

bool SpaceView::drawViewBg(QPainter& painter, QColor& bg_out, const FileEntryView& file, bool fillDir)
{
    auto rect=file.get_draw_area();

    if(rect.w<5 || rect.h<5)
        return false;

    QRect qr {rect.x, rect.y, rect.w, rect.h};

    QColor fillColor, strokeColor;
    bool isHovered = hoveredId == file.getId();
    bool isScanning = currentScannedId == file.getId();

    switch(file.get_type())
    {
        case FileEntryView::EntryType::AVAILABLE_SPACE:
            fillColor=customPalette.getViewAvailableFill();
            strokeColor=customPalette.getViewAvailableLine();
            break;
        case FileEntryView::EntryType::DIRECTORY:
            fillColor=customPalette.getViewDirFill();
            strokeColor=customPalette.getViewDirLine();
            break;
        case FileEntryView::EntryType::FILE:
            fillColor=customPalette.getViewFileFill();
            strokeColor=customPalette.getViewFileLine();
            break;
        default:
            fillColor=customPalette.getViewUnknownFill();
            strokeColor=customPalette.getViewUnknownLine();
            break;
    }

    if(isHovered)
    {
        fillColor = customPalette.bgBlend(fillColor, 0.7);
        strokeColor = customPalette.bgBlend(strokeColor, 0.5);
    } else if(isScanning && !isPaused())
    {
        fillColor = customPalette.bgBlend(fillColor, 0.5);
        strokeColor = customPalette.bgBlend(strokeColor, 0.3);
    }

    bg_out = fillColor;

    if(fillDir || isHovered || isScanning)
        painter.setBrush(QBrush(fillColor));
    else
        painter.setBrush(Qt::NoBrush);

    painter.setPen(QPen(strokeColor));
    painter.drawRect(qr);

    return true;
}

void SpaceView::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        if(scanner && viewDB->isReady())
        {
            if(hoveredPath)
            {
                auto path = Utils::make_unique<FilePath>(*hoveredPath);
                entryPopup->updateActions(*scanner);
                entryPopup->popup(std::move(path));
                fileTooltip->hideTooltip();
            }
        } else if(onNewScanRequestCallback)
            onNewScanRequestCallback();

        if(onActionCallback)
            onActionCallback();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void SpaceView::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();

    mouseX=event->x();
    mouseY=event->y();

    if(updateHoveredView())
        repaint();

}

bool SpaceView::updateHoveredView()
{
    auto hoveredEntry = viewDB->getHoveredView(mouseX, mouseY);
    auto prevHoveredId = hoveredId;
    if(!hoveredEntry)
    {
        fileTooltip->hideTooltip();
        hoveredPath.reset();
        hoveredId = 0;
        return prevHoveredId != 0;
    }

    hoveredId = hoveredEntry->getId();
    if(!hoveredEntry->get_parent() || (!hoveredEntry->is_dir() && !hoveredEntry->is_file()))
    {
        //don't show tooltips for views without parent and non-dirs/non-files
        fileTooltip->hideTooltip();
        hoveredPath.reset();
        hoveredId = 0;
        return prevHoveredId != hoveredId;
    }

    if(!hoveredPath)
        hoveredPath = Utils::make_unique<FilePath>(*currentPath);
    else
        *hoveredPath = *currentPath;

    hoveredEntry->getPath(*hoveredPath);

    auto point = mapToGlobal(QPoint(mouseX, mouseY));
    fileTooltip->setTooltip(point.x(), point.y(), hoveredEntry->get_tooltip());

    return prevHoveredId != hoveredId;
}

void SpaceView::updateScannedView()
{
    currentScannedId = 0;
    if(currentScannedPath)
    {
        auto view = viewDB->getClosestView(*currentScannedPath, currentDepth+1);
        if(view)
            currentScannedId = view->getId();
    }
}

void SpaceView::setOnActionCallback(std::function<void(void)> callback)
{
    onActionCallback = std::move(callback);
}

void SpaceView::setOnWatchLimitCallback(std::function<void(void)> callback)
{
    onWatchLimitCallback = std::move(callback);
}

void SpaceView::setOnNewScanRequestCallback(std::function<void(void)> callback)
{
    onNewScanRequestCallback = std::move(callback);
}

void SpaceView::setScanner(std::unique_ptr<SpaceScanner> _scanner)
{
    if(scanner)
        scanner->stop_scan();

    scanner=std::move(_scanner);
    clearHistory();
    if(scanner)
    {
        currentPath = Utils::make_unique<FilePath>(*(scanner->getRootPath()));
        viewDB->setFileDB(scanner->getFileDB());
        historyPush();
    }
    else
    {
        currentPath.reset();
        viewDB->setFileDB(nullptr);
    }
    onScanUpdate();
}

void SpaceView::setCustomPalette(const CustomPalette& palette)
{
    customPalette = palette;
    viewDB->onThemeChanged();
    allocateEntries();
}

bool SpaceView::getWatcherLimits(int64_t& watchedNow, int64_t& watchLimit)
{
    if(!scanner)
        return false;

    return scanner->getWatcherLimits(watchedNow, watchLimit);
}

bool SpaceView::isAtRoot()
{
    if(!scanner)
        return true;
    return !currentPath->canGoUp();
}

bool SpaceView::canRefresh()
{
    if(!scanner)
        return false;
    return scanner->can_refresh();
}

void SpaceView::rescanDir(const FilePath& dir_path)
{
    scanner->rescan_dir(dir_path);
}

void SpaceView::rescanCurrentView()
{
    if(!scanner || ! currentPath)
        return;

    //TODO if not at root, history is saved after rescan. but navigatin through it may
    // be not possible. We should handle this somehow
    if(isAtRoot())
    {
        clearHistory();
        historyPush();
    }
    rescanDir(*currentPath);
}

bool SpaceView::isScanOpen()
{
    return viewDB->isReady() && scanner;
}

int SpaceView::getScanProgress()
{
    if(scanner)
        return scanner->get_scan_progress();
    return 100;
}

bool SpaceView::isProgressKnown()
{
    if(scanner)
        return scanner->isProgressKnown();
    return false;
}

bool SpaceView::getSpace(uint64_t& scannedVisible, uint64_t& scannedHidden, uint64_t& available, uint64_t& total)
{
    if(!scanner)
        return false;

    uint64_t scanned;
    scanner->getSpace(scanned, available, total);

    scannedHidden=0;
    scannedVisible = viewDB->getFilesSize();
    if(scannedVisible>scanned)
        scannedVisible=scanned;
    if(scannedVisible<scanned && !isAtRoot())
        scannedHidden = scanned-scannedVisible;

    return true;
}

int64_t SpaceView::getScannedFiles()
{
    if(scanner)
        return scanner->getFileCount();
    return 0;
}

int64_t SpaceView::getScannedDirs()
{
    if(scanner)
        return scanner->getDirCount();
    return 0;
}

void SpaceView::setShowFreeSpace(bool showAvailable_)
{
    showAvailable = showAvailable_;
    onScanUpdate();
}

void SpaceView::setShowUnknownSpace(bool showUnknown_)
{
    showUnknown = showUnknown_;
    onScanUpdate();
}

void SpaceView::historyPush()
{
    while(!pathHistory.empty() && (pathHistoryPointer+1)<pathHistory.size())
        pathHistory.pop_back();

    pathHistory.push_back(Utils::make_unique<FilePath>(*currentPath));
    pathHistoryPointer = pathHistory.size()-1;
}

void SpaceView::clearHistory()
{
    pathHistory.clear();
    pathHistoryPointer = 0;
}

void SpaceView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    auto sz = size();

    Utils::RectI rect{};
    int padding=0;
    rect.x=padding;
    rect.y=padding;
    rect.w=sz.width()-2*padding-1;
    rect.h=sz.height()-2*padding-1;

    viewDB->setViewArea(rect);
    allocateEntries();
}

void SpaceView::onScanUpdate()
{
    if(scanner)
    {
        scanner->getCurrentScanPath(currentScannedPath);
        allocateEntries();
        entryPopup->updateActions(*scanner);

        int64_t watchedNow, watchLimit;
        if(scanner->getWatcherLimits(watchedNow, watchLimit) && onWatchLimitCallback)
            onWatchLimitCallback();
    }
    repaint();
}

void SpaceView::allocateEntries()
{
    if(viewDB->isReady() && currentPath)
    {
        viewDB->setViewDepth(currentDepth);
        viewDB->setViewPath(*currentPath);
        viewDB->setTextHeight(textHeight);
        viewDB->update(showUnknown, showAvailable);

        updateHoveredView();
        updateScannedView();
    }
}

void SpaceView::drawViewTitle(QPainter& painter, const QColor& bg, const FileEntryView& file)
{
    Utils::RectI rect=file.get_draw_area();
    if(rect.w < textHeight || rect.h < textHeight)
        return;

    auto col = CustomPalette::getTextColorFor(bg);
    auto titlePix = file.getTitlePixmap(painter, col,
            file.get_parent() ? nullptr : currentPath->getPath().c_str());

    QRect rt{textHeight / 2 + rect.x, rect.y,
             rect.w - textHeight, (textHeight * 3) / 2};
    int dy = (rt.height() - titlePix.height())/2;

    painter.setClipRect(rt);
    painter.drawPixmap(rt.x(), rect.y+dy, titlePix);
    painter.setClipping(false);
}

void SpaceView::drawViewText(QPainter &painter, const QColor& bg, const FileEntryView& file)
{
    auto rect=file.get_draw_area();
    if(rect.w < textHeight || rect.h < textHeight)
        return;
    //Strategy:
    //if enough space: display both name and size centered
    //if height not enough - display only name
    //if even name doesn't fit - display nothing
    //if width not enough - don't center text, just start it from the left border

    auto col = CustomPalette::getTextColorFor(bg);

    auto namePix = file.getNamePixmap(painter, col);
    auto sizePix = file.getSizePixmap(painter, col);

    int lineHeight = painter.fontMetrics().height();
    bool showSize = true;

    if(lineHeight > rect.h) //we can't show anything
        return;
    else if(lineHeight*2 > rect.h) //show only name
        showSize = false;

    QRect rt{2+rect.x, 2+rect.y,
             rect.w-4, rect.h-4};

    painter.setClipRect(rt);

    int dx1=2;
    if(namePix.width()<=rt.width())
        dx1 = (rt.width() - namePix.width())/2;

    if(showSize)
    {
        int dy2 = rt.height()/2;
        int dy1 = dy2 - namePix.height();
        painter.drawPixmap(rect.x+dx1,rect.y+dy1,namePix);

        int dx2=2;
        if(sizePix.width()<=rt.width())
            dx2 = (rt.width() - sizePix.width())/2;
        painter.drawPixmap(rect.x+dx2,rect.y+dy2,sizePix);
    }
    else
    {
        int dy1 = (rt.height() - namePix.height())/2;
        painter.drawPixmap(rect.x+dx1,rect.y+dy1,namePix);
    }
    painter.setClipping(false);
}

void SpaceView::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    mouseX = -1;
    mouseY = -1;
    repaint();
}

void SpaceView::navigateHome()
{
    while(currentPath->canGoUp())
        currentPath->goUp();
    historyPush();
    onScanUpdate();
}

bool SpaceView::navigateUp()
{
    if(isAtRoot())
        return false;

    currentPath->goUp();
    historyPush();
    onScanUpdate();
    return !isAtRoot();
}

void SpaceView::navigateBack()
{
    if(canNavigateBack())
    {
        --pathHistoryPointer;
        currentPath=Utils::make_unique<FilePath>(*pathHistory[pathHistoryPointer]);
        onScanUpdate();
    }

}

void SpaceView::navigateForward()
{
    if(canNavigateForward())
    {
        //TODO if scanner can't give information about this new path, we should do something
        // this might happen if we navigated to some folder, then moved back, folder was deleted and scanner updated
        // its info so there is no such folder anymore
        // the same goes to navigateBack
        ++pathHistoryPointer;
        currentPath=Utils::make_unique<FilePath>(*pathHistory[pathHistoryPointer]);
        onScanUpdate();
    }
}

bool SpaceView::canNavigateBack()
{
    return !pathHistory.empty() &&
           (pathHistoryPointer>0 && pathHistoryPointer < (pathHistory.size()+1));
}

bool SpaceView::canNavigateForward()
{
    return !pathHistory.empty() &&
           (pathHistoryPointer+1>=0 && pathHistoryPointer+1 < pathHistory.size());
}

bool SpaceView::canIncreaseDetail()
{
    return viewDB->isReady() && currentDepth<MAX_DEPTH;
}

bool SpaceView::canDecreaseDetail()
{
    return viewDB->isReady() && currentDepth>MIN_DEPTH;
}

bool SpaceView::canTogglePause()
{
    if(!scanner)
        return false;

    return scanner->canPause() != scanner->canResume();
}

bool SpaceView::isPaused()
{
    if(!scanner)
        return true;

    return scanner->canResume();
}

void SpaceView::setPause(bool paused)
{
    if(!scanner)
        return;

    if(paused)
        scanner->pauseScan();
    else
        scanner->resumeScan();
}

void SpaceView::increaseDetail()
{
    if(canIncreaseDetail())
    {
        ++currentDepth;
        onScanUpdate();
    }
}

void SpaceView::decreaseDetail()
{
    if(canDecreaseDetail())
    {
        --currentDepth;
        onScanUpdate();
    }
}

void SpaceView::mousePressEvent(QMouseEvent *event) {

    if(event->button() == Qt::LeftButton)
    {
        if(hoveredPath)
        {
            currentPath = Utils::make_unique<FilePath>(*hoveredPath);
            historyPush();
            event->accept();
            onScanUpdate();

            if(onActionCallback)
                onActionCallback();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

