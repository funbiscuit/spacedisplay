
#include <QtWidgets>
#include <vector>
#include <iostream>
#include "spaceview.h"
#include "colortheme.h"
#include "mainwindow.h"
#include "spacescanner.h"
#include "fileentrypopup.h"
#include "resources.h"
#include "resource_builder/resources.h"

SpaceView::SpaceView(MainWindow* parent) : QWidget(), parent(parent)
{
    setMouseTracking(true);
    entryPopup = Utils::make_unique<FileEntryPopup>(this);

    entryPopup->onRescanListener = [this](const std::string& str)
    {
        rescanDir(str);
        this->parent->updateAvailableActions();
        this->parent->updateStatusView();
    };

    auto& r = Resources::get();
    bgIcon = r.get_vector_pixmap(
            ResourceBuilder::RES___ICONS_SVG_APPICON_BW_SVG,
            256);

}

void SpaceView::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    int width = size().width();
    int height = size().height();

    if(root)
    {
        auto fm = painter.fontMetrics();

        textHeight = fm.height();
        painter.fillRect(0, 0, width, height, colorTheme->background);
//        Utils::tic();
        drawView(painter, root, currentDepth, true);
//        Utils::toc();
    } else {
        int x0=(width-bgIcon.width())/2;
        int y0=(height-bgIcon.height())/2;
        painter.drawPixmap(x0,y0, bgIcon);
        auto col = colorTheme->background;
        col.setAlpha(150);
        painter.fillRect(0, 0, width, height, col);
    }
}

void SpaceView::drawView(QPainter& painter, const FileEntrySharedPtr &file, int nestLevel, bool forceFill)
{
    QColor bg;
    if(!drawViewBg(painter, bg, file, forceFill || file->get_type() != FileEntryShared::EntryType::DIRECTORY))
        return;


    if(nestLevel>0 && file->get_type()==FileEntryShared::EntryType::DIRECTORY && !file->get_children().empty())
    {
        drawViewTitle(painter, bg, file);
        for(const auto& child : file->get_children())
        {
            drawView(painter, child, nestLevel - 1, false);
        }
    } else
    {
        drawViewText(painter, bg, file);
    }
}

bool SpaceView::drawViewBg(QPainter& painter, QColor& bg_out, const FileEntrySharedPtr &file, bool fillDir)
{
    auto rect=file->get_draw_area();

    if(rect.w<5 || rect.h<5)
        return false;

    QRect qr {rect.x, rect.y, rect.w, rect.h};

    QColor fillColor, strokeColor;

    switch(file->get_type())
    {
        case FileEntryShared::EntryType::AVAILABLE_SPACE:
            fillColor=colorTheme->viewFreeFill;
            strokeColor=colorTheme->viewFreeLine;
            break;
        case FileEntryShared::EntryType::DIRECTORY:
            fillColor=colorTheme->viewDirFill;
            strokeColor=colorTheme->viewDirLine;
            break;
        case FileEntryShared::EntryType::FILE:
            fillColor=colorTheme->viewFileFill;
            strokeColor=colorTheme->viewFileLine;
            break;
        default:
            fillColor=colorTheme->viewUnknownFill;
            strokeColor=colorTheme->viewUnknownLine;
            break;
    }

    if(file->isHovered && (!fillDir || file->get_type()!=FileEntryShared::EntryType::DIRECTORY))
    {
        fillColor = colorTheme->tint(fillColor, 0.7f);
        strokeColor = colorTheme->tint(strokeColor, 0.5f);
    }

    bg_out = fillColor;

    if(fillDir || file->isParentHovered)
        painter.setBrush(QBrush(fillColor));
    else
        painter.setBrush(Qt::NoBrush);

    painter.setPen(QPen(strokeColor));
    painter.drawRect(qr);

    return true;
}

void SpaceView::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::RightButton && parent)
    {
        if(scanner && root)
        {
            auto hovered = getHoveredEntry();
            if(hovered)
            {
                std::string entryPath;
                if(currentPath.length()>0)
                {
                    if(currentPath.back()!='/' && currentPath.back()!='\\')
                        currentPath.append("/");
                    entryPath = currentPath + hovered->get_path(false);
                }
                else
                    entryPath = hovered->get_path();

                entryPopup->updateActions(scanner);
                if(hovered->is_dir())
                    entryPopup->popupDir(entryPath);
                else
                    entryPopup->popupFile(entryPath);
            }
        } else
        {
            parent->newScan();
        }
        parent->updateAvailableActions();
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

    auto prevHovered = hoveredEntry;
    if(hoveredEntry)
    {
        hoveredEntry->unhover();
        hoveredEntry= nullptr;
    }

    if(updateHoveredView(prevHovered))
        repaint();

}

FileEntryShared* SpaceView::getHoveredEntry()
{
    if(hoveredEntry && (hoveredEntry->is_dir() || hoveredEntry->is_file()))
        return hoveredEntry;
    return nullptr;
}


bool SpaceView::updateHoveredView(FileEntryShared* prevHovered)
{
    if(root)
        hoveredEntry = root->update_hovered_element(mouseX, mouseY);
    else
        return false;

    bool updated = prevHovered!=hoveredEntry;

    static FileEntryShared* tooltipEntry = nullptr;
//return;
    //TODO show tooltip after small delay (when hovered entry stays the same for some time)
    if(hoveredEntry)
    {
        if(!hoveredEntry->get_parent() || (!hoveredEntry->is_dir() && !hoveredEntry->is_file()))
        {
            //don't show tooltips for views without parent and non-dirs/non-files
            tooltipEntry = nullptr;
            if(QToolTip::isVisible())
                QToolTip::hideText();
            return updated;
        }
        if(tooltipEntry != hoveredEntry)
        {
//            std::cout << "Show for: "<<hoveredEntry->get_name()<<"\n";
            tooltipEntry = hoveredEntry;
            QToolTip::showText(mapToGlobal(QPoint(mouseX, mouseY)),
                               hoveredEntry->get_tooltip().c_str());
        }
    } else if(QToolTip::isVisible())
    {
//        std::cout << "hide: "<<QToolTip::text().toStdString().c_str()<<"\n";
        tooltipEntry = nullptr;
        QToolTip::hideText();
    }

    return updated;
}

void SpaceView::setScanner(SpaceScanner* _scanner)
{
    currentPath = _scanner->get_root_path();
    historyPush();
    scanner=_scanner;
    onScanUpdate();
}

void SpaceView::setTheme(std::shared_ptr<ColorTheme> theme)
{
    colorTheme = theme;
}

bool SpaceView::isAtRoot()
{
    if(!scanner)
        return true;

    auto rootPath = scanner->get_root_path();
    auto rootLen = strlen(rootPath);
    //already at root
    if(currentPath.length() <= rootLen)
        return true;

    auto rel_path = currentPath.substr(rootLen);
    while(rel_path[0]=='/')
        rel_path = rel_path.substr(1);

    return rel_path.length()==0;
}

void SpaceView::rescanDir(const std::string &dir_path)
{
    std::cout << "Rescan: "<<dir_path<<"\n";
    scanner->rescan_dir(dir_path);
}

void SpaceView::setShowFreeSpace(bool showFree)
{
    if(showFree)
        fileEntryShowFlags |= FileEntryShared::INCLUDE_AVAILABLE_SPACE;
    else
        fileEntryShowFlags &= ~FileEntryShared::INCLUDE_AVAILABLE_SPACE;
    onScanUpdate();
}

void SpaceView::setShowUnknownSpace(bool showUnknown)
{
    if(showUnknown)
        fileEntryShowFlags |= FileEntryShared::INCLUDE_UNKNOWN_SPACE;
    else
        fileEntryShowFlags &= ~FileEntryShared::INCLUDE_UNKNOWN_SPACE;
    onScanUpdate();
}

void SpaceView::historyPush()
{
    while(!pathHistory.empty() && pathHistoryPointer<pathHistory.size()-1)
        pathHistory.pop_back();

    pathHistory.push_back(currentPath);
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
    allocateEntries();
}

void SpaceView::onScanUpdate()
{
    allocateEntries();
    repaint();
    entryPopup->updateActions(scanner);
}

void SpaceView::allocateEntries()
{
    auto sz = size();

    Utils::RectI rect{};
    int padding=0;
    rect.x=padding;
    rect.y=padding;
    rect.w=sz.width()-2*padding-1;
    rect.h=sz.height()-2*padding-1;

    if(scanner!=nullptr)
    {
        auto fullArea=float(rect.w*rect.h);
        float minArea=7.f*7.f;//minimum area of displayed object

        if(hoveredEntry)
        {
            hoveredEntry->unhover();
            hoveredEntry= nullptr;
        }

        if(root)
            scanner->update_root_file(root, minArea/fullArea, fileEntryShowFlags,
                                      currentPath.c_str(), currentDepth);
        else
            root=scanner->get_root_file(minArea/fullArea, fileEntryShowFlags,
                                        currentPath.c_str(), currentDepth);

        if(root)
        {
            //extra padding
            root->allocate_children(rect, (textHeight * 3) / 2);
        }

        updateHoveredView();
    }
}

void SpaceView::drawViewTitle(QPainter& painter, const QColor& bg, const FileEntrySharedPtr &file)
{
    Utils::RectI rect=file->get_draw_area();
    if(rect.w < textHeight || rect.h < textHeight)
        return;

    auto col = colorTheme->textFor(bg);
    auto titlePix = file->getTitlePixmap(painter, col);

    QRect rt{textHeight / 2 + rect.x, rect.y,
             rect.w - textHeight, (textHeight * 3) / 2};
    int dy = (rt.height() - titlePix.height())/2;

    painter.setClipRect(rt);
    painter.drawPixmap(rt.x(), rect.y+dy, titlePix);
    painter.setClipping(false);
}

void SpaceView::drawViewText(QPainter &painter, const QColor& bg, const FileEntrySharedPtr &file)
{
    auto rect=file->get_draw_area();
    if(rect.w < textHeight || rect.h < textHeight)
        return;
    //Strategy:
    //if enough space: display both name and size centered
    //if height not enough - display only name
    //if even name doesn't fit - display nothing
    //if width not enough - don't center text, just start it from the left border

    auto col = colorTheme->textFor(bg);

    auto namePix = file->getNamePixmap(painter, col);
    auto sizePix = file->getSizePixmap(painter, col);

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
    if(hoveredEntry)
    {
        hoveredEntry->unhover();
        hoveredEntry= nullptr;
    }
    repaint();
}

uint64_t SpaceView::getHiddenSize()
{
    if(!root || !scanner || isAtRoot())
        return 0;

    auto scanned = (int64_t) scanner->get_scanned_space();
    if(scanned<=root->get_size())
        return 0;
    return scanned-root->get_size();
}

bool SpaceView::navigateUp()
{
    if(isAtRoot())
        return false;

    auto rootPath = scanner->get_root_path();
    auto rootLen = strlen(rootPath);
    auto newPath = Utils::get_parent_path(currentPath);

    if(newPath.length()>=rootLen)
    {
        currentPath = newPath;
        for(size_t i=0;i<rootLen;++i)
            currentPath[i]=rootPath[i];

        historyPush();
        onScanUpdate();
        return !isAtRoot();
    }

    return false;
}

void SpaceView::navigateBack()
{
    if(canNavigateBack())
    {
        --pathHistoryPointer;
        std::cout << "Go back to: "<<pathHistory[pathHistoryPointer]<<"\n";
        currentPath=pathHistory[pathHistoryPointer];
        onScanUpdate();
    }

}

void SpaceView::navigateForward()
{
    if(canNavigateForward())
    {
        ++pathHistoryPointer;
        std::cout << "Go forward to: "<<pathHistory[pathHistoryPointer]<<"\n";
        currentPath=pathHistory[pathHistoryPointer];
        onScanUpdate();
    }
}

bool SpaceView::canNavigateBack()
{
    return !pathHistory.empty() &&
           (pathHistoryPointer-1>=0 && pathHistoryPointer-1 < pathHistory.size());
}

bool SpaceView::canNavigateForward()
{
    return !pathHistory.empty() &&
           (pathHistoryPointer+1>=0 && pathHistoryPointer+1 < pathHistory.size());
}

bool SpaceView::canIncreaseDetail()
{
    return root && currentDepth<MAX_DEPTH;
}

bool SpaceView::canDecreaseDetail()
{
    return root && currentDepth>MIN_DEPTH;
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
        if(hoveredEntry && hoveredEntry->is_dir() && hoveredEntry->get_parent() != nullptr)
        {
            currentPath = currentPath + hoveredEntry->get_path(false) + "/";
            historyPush();
            event->accept();
            onScanUpdate();
            if(parent)
            {
                parent->updateAvailableActions();
                parent->updateStatusView();
            }
            return;
        }
    }
//    else if(event->button() == Qt::RightButton)
//    {
//        if(hoveredEntry && (hoveredEntry->is_dir() || hoveredEntry->is_file() ))
//        {
//            event->accept();
//
//            auto currentPath = getCurrentPath();
//            std::string entryPath;
//            if(currentPath.length()>0)
//            {
//                if(currentPath.back()!='/' && currentPath.back()!='\\')
//                    currentPath.append("/");
//                entryPath = currentPath + hoveredEntry->get_path(false);
//            }
//            else
//                entryPath = hoveredEntry->get_path();
//
//            showEntryPopup(entryPath, hoveredEntry->is_dir())
//            return;
//        }
//    }
    QWidget::mousePressEvent(event);
}

