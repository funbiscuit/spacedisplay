
#include <QtWidgets>
#include <vector>
#include <iostream>
#include "spaceview.h"
#include "mainwindow.h"
#include "spacescanner.h"
#include "fileentrypopup.h"
#include "resources.h"
#include "resource_builder/resources.h"

SpaceView::SpaceView() : QWidget()
{
    setMouseTracking(true);
    entryPopup = std::make_unique<FileEntryPopup>(this);


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
        painter.fillRect(0, 0, width, height, Qt::lightGray);
        Utils::tic();
        drawView(painter, root, currentDepth, true);
        Utils::toc();
    } else {
        int x0=(width-bgIcon.width())/2;
        int y0=(height-bgIcon.height())/2;
        painter.drawPixmap(x0,y0, bgIcon);
        painter.fillRect(0, 0, width, height, QColor::fromRgbF(1.0,1.0,1.0,0.6));
    }
}

void SpaceView::drawView(QPainter& painter, const FileEntrySharedPtr &file, int nestLevel, bool forceFill)
{
    if(!drawViewBg(painter, file, forceFill || file->get_type() != FileEntry::DIRECTORY))
        return;


    if(nestLevel>0 && file->get_type()==FileEntry::DIRECTORY && !file->get_children().empty())
    {
        drawViewTitle(painter, file);
        for(const auto& child : file->get_children())
        {
            drawView(painter, child, nestLevel - 1, false);
        }
    } else
    {
        drawViewText(painter, file);
    }
}

bool SpaceView::drawViewBg(QPainter& painter, const FileEntrySharedPtr &file, bool fillDir)
{
    Utils::Rect rect=file->get_draw_area();

    if(rect.w<5.f || rect.h<5.f)
        return false;

    //TODO should use integers from the start
    QRect qr {int(rect.x), int(rect.y), int(rect.w), int(rect.h)};

    int rgbF[3];//for fill
    int rgbS[3];//for stroke

    int fillColor, strokeColor;

    switch(file->get_type())
    {
        case FileEntry::AVAILABLE_SPACE:
            fillColor=0x77dd77;
            strokeColor=0x5faf5f;//80% value of 0x77dd77
            break;
        case FileEntry::DIRECTORY:
            fillColor=0xffdead;
            strokeColor=0xccb08a;//80% value of 0xffdead
            break;
        case FileEntry::FILE:
            fillColor=0x779ecb;
            strokeColor=0x5e7da0;//80% value of 0x779ecb
            break;
        default:
            fillColor=0xcfcfc4;
            strokeColor=0xa5a59d;//80% value of 0xcfcfc4
            break;
    }

    if(file->isHovered && (!fillDir || file->get_type()!=FileEntry::DIRECTORY))
    {
        hex_to_rgbi_tint(fillColor,rgbF,0.9f);
        hex_to_rgbi_tint(strokeColor,rgbS,0.5f);
    }
    else
    {
        hex_to_rgbi(fillColor,rgbF);
        hex_to_rgbi(strokeColor,rgbS);
    }

    if(fillDir || file->isParentHovered)
        painter.setBrush(QBrush(QColor(rgbF[0],rgbF[1],rgbF[2])));
    else
        painter.setBrush(Qt::NoBrush);

    painter.setPen(QPen(QColor(rgbS[0],rgbS[1],rgbS[2])));
    painter.drawRect(qr);

    return true;
}

void SpaceView::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        auto parent = dynamic_cast<MainWindow*>(parentWidget());
//        bool isWidgetHovered = spaceWidget->rect().contains(spaceWidget->mapFromGlobal(event->globalPos()));
        if(parent)
        {
            if(scanner && root)
            {
                auto hovered = getHoveredEntry();
                if(hovered)
                {
                    entryPopup->updateActions(scanner);
                    if(hovered->is_dir())
                        entryPopup->popupDir(hovered->get_path());
                    else
                        entryPopup->popupFile(hovered->get_path());
                }
            } else
            {
                parent->newScan();
            }
            parent->updateAvailableActions();
            event->accept();
            return;
        }
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

    Utils::Rect rect{};
    float padding=0.f;
    rect.x=std::round(padding);
    rect.y=std::round(padding);
    rect.w=std::round(sz.width()-2*padding);
    rect.h=std::round(sz.height()-2*padding);

    if(scanner!=nullptr)
    {
        float fullArea=rect.w*rect.h;
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

void SpaceView::drawViewTitle(QPainter& painter, const FileEntrySharedPtr &file)
{
    Utils::Rect rect=file->get_draw_area();
    if(rect.w < textHeight || rect.h < textHeight)
        return;

    auto title = file->get_title();

    QRect rt{textHeight / 2 + int(rect.x), int(rect.y),
             int(rect.w) - textHeight, (textHeight * 3) / 2};

    int rgb[3];
    hex_to_rgbi(0x3b3b3b,rgb);

//    return;
    painter.setPen(QColor(rgb[0],rgb[1],rgb[2]));
    painter.setClipRect(rt);
    painter.drawText(rt, Qt::AlignVCenter, title.c_str());
    painter.setClipping(false);
}

void SpaceView::drawViewText(QPainter &painter, const FileEntrySharedPtr &file)
{
    Utils::Rect rect=file->get_draw_area();
    if(rect.w < textHeight || rect.h < textHeight)
        return;
    //Strategy:
    //if enough space: display both name and size centered
    //if height not enough - display only name
    //if even name doesn't fit - display nothing
    //if width not enough - don't center text, just start it from the left border

    std::string textNameOnly = file->get_name();
    std::string textSizeOnly = file->format_size();

    std::string textFull = string_format("%s\n%s", textNameOnly.c_str(), textSizeOnly.c_str());

    int align = Qt::AlignCenter;

//    return;
    QSize nameSize = painter.fontMetrics().size(0, textNameOnly.c_str());
    QSize sizeSize = painter.fontMetrics().size(0, textSizeOnly.c_str());

    int lineHeight = painter.fontMetrics().height();
//    return;
    bool showSize = true;

    if(lineHeight > rect.h) //we can't show anything
        return;
    else if(lineHeight*2 > rect.h) //show only name
        showSize = false;

    QRect rt{2+int(rect.x), 2+int(rect.y),
             int(rect.w)-4, int(rect.h)-4};
    int rgb[3];
    hex_to_rgbi(0x3b3b3b,rgb);

    painter.setClipRect(rt);
    painter.setPen(QColor(rgb[0],rgb[1],rgb[2]));


    if(showSize)
    {
        QRect rt_top{2+int(rect.x), 2+int(rect.y),
                     int(rect.w)-4, rt.height()/2};
        QRect rt_bot{2+int(rect.x), rt_top.y()+rt_top.height(),
                     int(rect.w)-4, rt.height()/2};
        align = Qt::AlignBottom;
        if(nameSize.width() <= rt.width())
            align = Qt::AlignBottom | Qt::AlignHCenter;
        painter.drawText(rt_top, align, textNameOnly.c_str());

        align = Qt::AlignTop;
        if(sizeSize.width() <= rt.width())
            align = Qt::AlignTop | Qt::AlignHCenter;
        painter.drawText(rt_bot, align, textSizeOnly.c_str());
    }
    else
    {
        align = Qt::AlignVCenter;
        if(nameSize.width() <= rt.width())
            align = Qt::AlignCenter;
        painter.drawText(rt, align, textNameOnly.c_str());
    }
    painter.setClipping(false);
}

void SpaceView::drawViewText2(QPainter &painter, const FileEntrySharedPtr &file)
{
    //this function uses caching
    //TODO implement caching correctly
    static std::unordered_map<uint64_t, QPixmap> cache;

    Utils::Rect rect=file->get_draw_area();
    if(rect.w < textHeight || rect.h < textHeight)
        return;


    //Strategy:
    //if enough space: display both name and size centered
    //if height not enough - display only name
    //if even name doesn't fit - display nothing
    //if width not enough - don't center text, just start it from the left border

    std::string textNameOnly = file->get_name();
    std::string textSizeOnly = file->format_size();

    std::string textFull = string_format("%s\n%s", textNameOnly.c_str(), textSizeOnly.c_str());

    int align = Qt::AlignCenter;

//    return;

    int lineHeight = painter.fontMetrics().height();
//    return;
    bool showSize = true;

    if(lineHeight > rect.h) //we can't show anything
        return;
    else if(lineHeight*2 > rect.h) //show only name
        showSize = false;

    QRect rt{2+int(rect.x), 2+int(rect.y),
             int(rect.w)-4, int(rect.h)-4};

    auto it = cache.find(file->get_id());
    if(it!=cache.end())
    {
        painter.drawPixmap(rt.x(), rt.y(), it->second);
//        std::cout << "use cache\n";
        return;
    }
    std::cout << "not use cache\n";

    QSize nameSize = painter.fontMetrics().size(0, textNameOnly.c_str());
    QSize sizeSize = painter.fontMetrics().size(0, textSizeOnly.c_str());

    int rgb[3];
    hex_to_rgbi(0x3b3b3b,rgb);

    QSize sz{int(rect.w), int(rect.h)};

    QPixmap pix(sz);
    pix.fill(Qt::transparent);
    QPainter painter_pix(&pix);
    painter_pix.setPen(QColor(rgb[0],rgb[1],rgb[2]));

    if(showSize)
    {
        QRect rt_top{2, 2,
                     int(rect.w)-4, rt.height()/2};
        QRect rt_bot{2, rt_top.y()+rt_top.height(),
                     int(rect.w)-4, rt.height()/2};
        align = Qt::AlignBottom;
        if(nameSize.width() <= rt.width())
            align = Qt::AlignBottom | Qt::AlignHCenter;
        painter_pix.drawText(rt_top, align, textNameOnly.c_str());

        align = Qt::AlignTop;
        if(sizeSize.width() <= rt.width())
            align = Qt::AlignTop | Qt::AlignHCenter;
        painter_pix.drawText(rt_bot, align, textSizeOnly.c_str());
    }
    else
    {
        QRect rt2{2, 2,
                  int(rect.w)-4, int(rect.h)-4};
        align = Qt::AlignVCenter;
        if(nameSize.width() <= rt.width())
            align = Qt::AlignCenter;
        painter_pix.drawText(rt2, align, textNameOnly.c_str());
    }

    cache.emplace(file->get_id(), pix);
//    cache.insert(std::p)
    painter.drawPixmap(rt.x(), rt.y(), pix);
//    painter.drawPixmap(0, 0, pix);

//    painter.setClipping(false);
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
            auto parent = dynamic_cast<MainWindow*>(parentWidget());
            if(parent)
                parent->updateAvailableActions();
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

