#include "customstyle.h"

#include "utils-gui.h"

#include <QStyleOption>
#include <QPainter>



void CustomStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption *option,
                                QPainter *painter, const QWidget *widget) const
{
    painter->save();
    if (element == PE_PanelButtonTool)
    {
        QRect tbRect = option->rect;
        tbRect.adjust(1,1,-2,-2);

        auto pushColFil = option->palette.button().color();
        auto hovColFil = pushColFil;
        hovColFil.setAlpha(127);

        double rectRad = tbRect.width() / 10.0;

        painter->setPen(Qt::NoPen);
        if((option->state & State_Enabled) != 0) {
            if (option->state & State_MouseOver && !(option->state & State_Sunken)) {
                painter->setBrush(hovColFil);
                painter->setRenderHint(QPainter::RenderHint::Antialiasing);
                painter->drawRoundedRect(tbRect, rectRad, rectRad);
            } else if (option->state & State_Sunken || option->state & State_On) {
                painter->setBrush(pushColFil);
                painter->setRenderHint(QPainter::RenderHint::Antialiasing);
                painter->drawRoundedRect(tbRect, rectRad, rectRad);
            }
        }
    } else if(element == PE_IndicatorToolBarSeparator) {
        painter->setPen(option->palette.mid().color());
        auto x1 = option->rect.x();
        auto h = option->rect.height();
        auto margin = h/6;
        auto y1 = option->rect.y() + margin;
        x1 += option->rect.width()/2;
        auto y2 = y1 + h - 2*margin;
        QRect tbRect = option->rect;
        painter->drawLine(x1,y1,x1,y2);
    }  else if(element == PE_PanelTipLabel) {
        auto r = option->rect;
        r.adjust(0,0,-1,-1);
        auto base = option->palette.toolTipBase().color();
        auto basePen = UtilsGui::blend(base,QColor(127,127,127),0.5);

        painter->setPen(basePen);
        painter->setBrush(option->palette.toolTipBase());
        painter->drawRect(r);
    } else if(element == PE_FrameMenu) {

    } else if(element == PE_PanelMenu) {
        auto r = option->rect;
        r.adjust(0,0,-1,-1);

        painter->setPen(option->palette.mid().color());
        painter->setBrush(option->palette.window());
        painter->drawRect(r);
    } else {
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
    painter->restore();
}

void CustomStyle::drawComplexControl(QStyle::ComplexControl control, const QStyleOptionComplex *option,
                                     QPainter *painter, const QWidget *widget) const
{
    if(control == QStyle::CC_ToolButton)
    {
        auto optionTb = (QStyleOptionToolButton*) option;
        if(optionTb)
        {

            QStyleOption optionPE;
            optionPE.rect = option->rect;
            optionPE.palette = option->palette;
            optionPE.state = option->state;
            auto pixmapSize = option->rect.size() * 0.8;
            auto mode = option->state & State_Enabled ? QIcon::Normal : QIcon::Disabled;
            auto pixmap = optionTb->icon.pixmap(pixmapSize, mode);

            drawPrimitive(PE_PanelButtonTool, &optionPE, painter, widget);
            drawItemPixmap(painter,option->rect, Qt::AlignCenter, pixmap);
        }

    } else
        QProxyStyle::drawComplexControl(control, option, painter, widget);
}

void CustomStyle::drawControl(QStyle::ControlElement control, const QStyleOption *option,
                              QPainter *painter, const QWidget *widget) const
{
    painter->save();

    if(control == CE_ToolBar)
    {
        QRect tbRect = option->rect;
        auto x1 = option->rect.x();
        auto x2 = x1 + option->rect.width();
        auto y = option->rect.y()+option->rect.height()-1;

        painter->setPen(option->palette.mid().color());
        painter->drawLine(x1, y, x2, y);
    } else if(control == CE_MenuItem)
    {
        auto menuOption = (QStyleOptionMenuItem*) option;
        if(menuOption)
        {
            auto r = option->rect;
            painter->setPen(Qt::NoPen);
            QColor textCol;
            if(menuOption->state & State_Selected && menuOption->state & State_Enabled)
            {
                textCol = option->palette.highlightedText().color();
                painter->setBrush(option->palette.highlight());
            } else
            {
                textCol = option->palette.windowText().color();
                if(!(menuOption->state & State_Enabled))
                    textCol.setAlpha(127);
                painter->setBrush(option->palette.window());
            }

            painter->drawRect(r);
            painter->setPen(textCol);
            r.adjust(r.height()/2,0,0,0);
            painter->drawText(r, Qt::AlignVCenter | Qt::AlignLeft, menuOption->text);
        }
    } else
        QProxyStyle::drawControl(control, option, painter, widget);

    painter->restore();
}

int CustomStyle::pixelMetric(QStyle::PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    //TODO calculate
    switch (metric)
    {
        case QStyle::PM_ToolTipLabelFrameWidth:
            return 9;
        case QStyle::PM_ToolBarItemSpacing:
            return 4;

        default: return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

