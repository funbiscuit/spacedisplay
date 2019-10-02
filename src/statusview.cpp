
#include <cmath>
#include <algorithm>
#include "utils.h"
#include "statusview.h"
#include "colortheme.h"

void StatusView::set_sizes(bool isAtRoot, float scannedVisible, float scannedHidden, float free, float unknown)
{
    if(isDirScanned)
    {
        free=0.f;
        unknown=0.f;
    }
    globalView = isAtRoot;
    scannedSpaceVisible=scannedVisible;
    scannedSpaceHidden=scannedHidden;
    freeSpace=free;
    unknownSpace=unknown;
}

void StatusView::set_progress(bool isReady, float progress)
{
    scanReady=isReady;
    scanProgress=progress;
}

void StatusView::setTheme(std::shared_ptr<ColorTheme> theme)
{
    colorTheme = theme;
}

std::string StatusView::get_text_status_progress()
{
    if(!scanReady)
    {
        // If we are scanning a directory, we don't actually know how much we need to scan
        // so don't display any numbers
        if(isDirScanned)
            return "Scanning...";
        int progress=(int)std::round(100.f*scanProgress);
        if(progress>99)
            progress=99;
        return string_format("%d%%", progress);
    }
    return "Ready";
}

void StatusView::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    int width = size().width()-1;
    int height = size().height()-1;

    int textMargin = 5;

    auto fm = painter.fontMetrics();
    auto str = get_text_status_progress();

    int availableWidth = width-fm.size(0, str.c_str()).width()-textMargin;

    QRect textRect{0,0,size().width(),size().height()};

    painter.setPen(QPen(colorTheme->foreground));
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignRight, str.c_str());

    //don't render actual bars if scan is not opened
    if(!scanOpened)
        return;

    parts.clear();

    StatusPart partScannedVisible, partScannedHidden, partFree, partUnknown;

    partScannedVisible.color = colorTheme->viewFileFill;
    partScannedVisible.isHidden = false;
    partScannedVisible.label = Utils::format_size((int64_t)scannedSpaceVisible);
    partScannedVisible.weight = scannedSpaceVisible;

    partScannedHidden.color = colorTheme->viewFileFill;
    partScannedHidden.isHidden = true;
    partScannedHidden.label = Utils::format_size((int64_t)scannedSpaceHidden);
    partScannedHidden.weight = scannedSpaceHidden;

    partFree.color = colorTheme->viewFreeFill;
    partFree.isHidden = !showFree;
    partFree.label = Utils::format_size((int64_t)freeSpace);
    partFree.weight = freeSpace;

    partUnknown.color = colorTheme->viewUnknownFill;
    partUnknown.isHidden = !showUnknown;
    partUnknown.label = Utils::format_size((int64_t)unknownSpace);
    partUnknown.weight = unknownSpace;

    parts.push_back(partScannedVisible);
    parts.push_back(partScannedHidden);
    parts.push_back(partFree);
    parts.push_back(partUnknown);

    allocate_parts(fm, float(availableWidth));


    int start=0;
    int end = availableWidth;
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
                auto bg = colorTheme->background;
                qreal r = 0.5;
                part.color = QColor::fromRgbF(
                        part.color.redF()*(1.0-r) + bg.redF()*r,
                        part.color.greenF()*(1.0-r) + bg.greenF()*r,
                        part.color.blueF()*(1.0-r) + bg.blueF()*r);
            }

            painter.fillRect(start1,0,barWidth,height, part.color);

            textRect.setX(start1);
            textRect.setWidth(barWidth);
            painter.setPen(QPen(colorTheme->textFor(part.color)));
            painter.drawText(textRect, Qt::AlignCenter, part.label.c_str());

            if(!part.isHidden)
                endAllVisible=end1;
        }
        start1+=barWidth;
    }

    painter.setPen( QColor(88,88,88));
    painter.setBrush(Qt::transparent);
    if(endAllVisible!=end1)
        painter.drawRect(0,0,endAllVisible, height);
}

void StatusView::allocate_parts(const QFontMetrics& fm, float width)
{
    std::vector<int> partLabelSizes;
    std::vector<float> minWeights;
    std::vector<float> availWeights;

    float totalWeight=0.f;
    for(auto it=parts.begin();it!=parts.end();)
    {
        //remove empty parts, we don't need them
        if(it->weight==0.f)
        {
            it=parts.erase(it);
            continue;
        }
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

        if(parts[i].weight>0.f)
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
