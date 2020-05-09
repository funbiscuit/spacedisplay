
#include <cmath>
#include <algorithm>
#include "utils.h"
#include "statusview.h"

void StatusView::setSpace(float scannedVisible, float scannedHidden, float available, float unknown)
{
    scannedSpaceVisible=scannedVisible;
    scannedSpaceHidden=scannedHidden;
    availableSpace=available;
    unknownSpace=unknown;
}

void StatusView::setProgress(int progress)
{
    scanProgress=progress;
}

void StatusView::setCustomPalette(const CustomPalette& palette)
{
    customPalette = palette;
}

void StatusView::setMode(Mode mode)
{
    currentMode = mode;
}

void StatusView::setSpaceHighlight(bool available, bool unknown)
{
    highlightAvailable = available;
    highlightUnknown = unknown;
}

void StatusView::setMaximizeSpace(bool maximize)
{
    maximizeSpace = maximize;
}

void StatusView::setScannedFiles(int64_t files)
{
    scannedFiles = files;
}

std::string StatusView::getScanStatusText()
{
    if(currentMode==Mode::NO_SCAN || currentMode==Mode::SCAN_FINISHED)
        return "Ready";
    // If we are scanning a directory, we don't actually know how much we need to scan
    // so don't display any numbers
    if(currentMode==Mode::SCANNING_INDEFINITE)
        return "Scanning...";

    // progress can't be 100% because then it will be ready
    if(scanProgress>99)
        scanProgress=99;
    return Utils::strFormat("%d%%", scanProgress);
}

std::string StatusView::getScannedFilesText()
{
    if(currentMode==Mode::NO_SCAN)
        return "";

    return Utils::strFormat("%d files", scannedFiles);
}

void StatusView::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    int width = size().width()-1;
    int height = size().height()-1;

    int textMargin = 5;

    auto fm = painter.fontMetrics();
    auto scanStatusStr = getScanStatusText();
    auto scannedFilesStr = getScannedFilesText();

    auto filesStrWidth = fm.size(0, scannedFilesStr.c_str()).width() + textMargin;

    int availableWidth = width - fm.size(0, scanStatusStr.c_str()).width() - textMargin - filesStrWidth;

    QRect textRect{0,0,size().width(),size().height()};

    painter.setPen(palette().windowText().color());
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignRight, scanStatusStr.c_str());
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, scannedFilesStr.c_str());

    //don't render actual bars if scan is not opened
    if(currentMode == Mode::NO_SCAN)
        return;

    parts.clear();

    StatusPart partScannedVisible, partScannedHidden, partFree, partUnknown;

    partScannedVisible.color = customPalette.getViewFileFill();
    partScannedVisible.isHidden = false;
    partScannedVisible.label = Utils::formatSize((int64_t) scannedSpaceVisible);
    partScannedVisible.weight = scannedSpaceVisible;

    partScannedHidden.color = customPalette.getViewFileFill();
    partScannedHidden.isHidden = true;
    partScannedHidden.label = Utils::formatSize((int64_t) scannedSpaceHidden);
    partScannedHidden.weight = scannedSpaceHidden;

    partFree.color = customPalette.getViewAvailableFill();
    partFree.isHidden = !highlightAvailable;
    partFree.label = Utils::formatSize((int64_t) availableSpace);
    partFree.weight = maximizeSpace ? -1.f : availableSpace;

    partUnknown.color = customPalette.getViewUnknownFill();
    partUnknown.isHidden = !highlightUnknown;
    partUnknown.label = Utils::formatSize((int64_t) unknownSpace);
    partUnknown.weight = maximizeSpace ? -1.f : unknownSpace;

    parts.push_back(partScannedVisible);
    parts.push_back(partScannedHidden);
    parts.push_back(partFree);
    parts.push_back(partUnknown);

    allocateParts(fm, float(availableWidth));


    int start = filesStrWidth;
    int end = start + availableWidth;
    int start1=start;
    int end1=end;
    int endAllVisible=end1;

    for(auto& part : parts)
    {
        int barWidth= static_cast<int>(float(availableWidth) * part.weight);
        end1 = start1 + barWidth;
        if(barWidth > 0)
        {
            if(part.isHidden)
            {
                //the same as just setting alpha, but we need actual final color to
                //correctly determine appropriate color for text
                part.color = customPalette.bgBlend(part.color, 0.5);
            }

            painter.fillRect(start1,0,barWidth,height, part.color);

            textRect.setX(start1);
            textRect.setWidth(barWidth);
            painter.setPen(CustomPalette::getTextColorFor(part.color));
            painter.drawText(textRect, Qt::AlignCenter, part.label.c_str());

            if(!part.isHidden)
                endAllVisible=end1;
        }
        start1+=barWidth;
    }

    painter.setPen(palette().windowText().color());
    painter.setBrush(Qt::transparent);
    if(endAllVisible!=end1)
        painter.drawRect(filesStrWidth,0,endAllVisible-filesStrWidth, height);
}

void StatusView::allocateParts(const QFontMetrics& fm, float width)
{
    // holds width of each label in pixels so we allocate enough space for it
    std::vector<int> partLabelSizes;
    // minimum weight of each part so each label is shown
    std::vector<float> minWeights;
    // holds weight of each part that is available (can be removed from part)
    // it will be negative if default weight is less than minimum weight
    std::vector<float> availWeights;

    float totalWeight=0.f;
    // calculate total weight of all parts, remove parts that should not be shown
    for(auto it=parts.begin();it!=parts.end();)
    {
        //remove empty parts, we don't need them
        if(it->weight==0.f)
        {
            it=parts.erase(it);
            continue;
        } else if(it->weight<0.f)
            it->weight=0.f; //this part should be minimized, but kept in place
        totalWeight+=it->weight;

        ++it;
    }

    minWeights.resize(parts.size());
    availWeights.resize(parts.size());
    partLabelSizes.resize(parts.size());

    float totalAvailable=0.f;
    float overdraw = -1.f;
    for(size_t i=0;i<parts.size();++i)
    {
        partLabelSizes[i]=fm.size(0,parts[i].label.c_str()).width();
        partLabelSizes[i]+=parts[i].textMargin*2;

        parts[i].weight/=totalWeight;
        minWeights[i]=partLabelSizes[i]/width;
        availWeights[i]=parts[i].weight - minWeights[i];

        if(parts[i].weight>=0.f)
        {
            if(availWeights[i]<0.f)
                parts[i].weight-=availWeights[i];
            else
                totalAvailable+=availWeights[i];
            overdraw+=parts[i].weight;
        }
    }

    float a = overdraw/totalAvailable;

    for(size_t i=0;i<parts.size();++i)
        if(availWeights[i]>0.f)
            parts[i].weight-=availWeights[i]*a;

    //sort parts in such a way that all shown are on left and all hidden are on right
    std::sort(parts.begin(), parts.end(), [](const StatusPart &lhs, const StatusPart &rhs) {
        return !lhs.isHidden && rhs.isHidden;
    });
}

QSize StatusView::minimumSizeHint() const
{
    return {0, fontMetrics().height()+textPadding*2};
}
