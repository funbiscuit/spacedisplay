#include "customstyle.h"

#include "utils-gui.h"
#include "utils.h"

#include <QtWidgets>


void CustomStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption *option,
                                QPainter *painter, const QWidget *widget) const {
    auto lh = option->fontMetrics.height();
    painter->save();
    if (element == PE_PanelButtonTool) {
        QRect tbRect = option->rect;
        tbRect.adjust(1, 1, -2, -2);

        auto pushColFil = option->palette.mid().color();
        auto hovColFil = pushColFil;
        hovColFil.setAlpha(127);

        double rectRad = tbRect.width() / 10.0;

        painter->setPen(Qt::NoPen);
        if ((option->state & State_Enabled) != 0) {
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
    } else if (element == PE_IndicatorToolBarSeparator) {
        painter->setPen(option->palette.midlight().color());
        auto x1 = option->rect.x();
        auto h = option->rect.height();
        auto margin = h / 6;
        auto y1 = option->rect.y() + margin;
        x1 += option->rect.width() / 2;
        auto y2 = y1 + h - 2 * margin;
        QRect tbRect = option->rect;
        painter->drawLine(x1, y1, x1, y2);
    } else if (element == PE_PanelTipLabel) {
        auto r = option->rect;
        r.adjust(0, 0, -1, -1);
        auto base = option->palette.toolTipBase().color();
        auto basePen = UtilsGui::blend(base, QColor(127, 127, 127), 0.5);

        painter->setPen(basePen);
        painter->setBrush(option->palette.toolTipBase());
        painter->drawRect(r);
    } else if (element == PE_PanelMenu) {
        auto r = option->rect;
        r.adjust(0, 0, -1, -1);

        painter->setPen(option->palette.midlight().color());
        painter->setBrush(option->palette.window());
        painter->drawRect(r);
    } else if (element == PE_PanelButtonCommand) {
        auto wid = Utils::roundFrac(lh, 10); //width of highlight around button
        double rectRad = Utils::roundFrac(lh, 9);
        QRectF r(option->rect);
        r.adjust(0.5, 0.5, -0.5, -0.5); //make it pixel-perfect
        auto btnR = r.adjusted(wid, wid, -wid, -wid);

        auto opt = (QStyleOptionButton *) option;

        painter->setRenderHint(QPainter::RenderHint::Antialiasing);

        if (option->state & State_HasFocus) {
            auto col = option->palette.highlight().color();
            painter->setPen(Qt::NoPen);
            col.setAlpha(127);
            painter->setBrush(col);
            painter->drawRoundedRect(r, rectRad, rectRad);
            col.setAlpha(255);
            painter->setPen(col);
        } else
            painter->setPen(option->palette.mid().color());


        auto fillCol = option->palette.button().color();
        if ((opt->features & QStyleOptionButton::DefaultButton)) {
            fillCol = option->palette.highlight().color();
            qreal h, s, v;
            fillCol.getHsvF(&h, &s, &v);
            if (UtilsGui::isDark(option->palette.window().color()))
                v *= 0.75;
            else
                s *= 0.75;
            fillCol.setHsvF(h, s, v);

            auto col = option->palette.highlight().color();
            painter->setPen(col);
        }

        if (!(option->state & State_Enabled))
            fillCol.setAlpha(127);

        painter->setBrush(fillCol);
        painter->drawRoundedRect(btnR, rectRad, rectRad);
    } else if (element == PE_PanelLineEdit) {
        auto wid = Utils::roundFrac(lh, 10); //width of highlight around edit
        QRectF r(option->rect);
        r.adjust(0.5, 0.5, -0.5, -0.5); //make it pixel-perfect

        painter->setRenderHint(QPainter::RenderHint::Antialiasing);

        if (option->state & State_HasFocus) {
            auto col = option->palette.highlight().color();
            painter->setPen(Qt::NoPen);
            col.setAlpha(127);
            painter->setBrush(col);
            painter->drawRoundedRect(r, 1, 1);
            col.setAlpha(255);
            painter->setPen(col);
        } else
            painter->setPen(option->palette.mid().color());

        auto fillCol = option->palette.button().color();

        if (!(option->state & State_Enabled))
            fillCol.setAlpha(127);

        painter->setBrush(fillCol);
        painter->drawRect(r.adjusted(wid, wid, -wid, -wid));
    } else if (element == PE_Frame) {
        QRectF r(option->rect);
        r.adjust(0.5, 0.5, -0.5, -0.5); //make it pixel-perfect

        painter->setRenderHint(QPainter::RenderHint::Antialiasing);

        if (option->state & State_HasFocus) {
            auto col = option->palette.highlight().color();
            painter->setPen(Qt::NoPen);
            col.setAlpha(127);
            painter->setBrush(col);
            painter->drawRoundedRect(r, 1, 1);
            col.setAlpha(255);
            painter->setPen(col);
        } else
            painter->setPen(option->palette.mid().color());
        r.adjust(1, 1, -1, -1);

        painter->setBrush(Qt::NoBrush);
        painter->drawRect(r);
    } else if (element == PE_FrameFocusRect ||
               element == PE_FrameMenu) {
        // skip elements
    } else {
        QCommonStyle::drawPrimitive(element, option, painter, widget);
    }
    painter->restore();
}

void CustomStyle::drawComplexControl(QStyle::ComplexControl control, const QStyleOptionComplex *option,
                                     QPainter *painter, const QWidget *widget) const {
    if (control == QStyle::CC_ToolButton) {
        auto optionTb = (QStyleOptionToolButton *) option;
        if (optionTb) {

            QStyleOption optionPE;
            optionPE.rect = option->rect;
            optionPE.palette = option->palette;
            optionPE.state = option->state;
            auto pixmapSize = option->rect.size() * 0.8;
            auto mode = option->state & State_Enabled ? QIcon::Normal : QIcon::Disabled;
            auto pixmap = optionTb->icon.pixmap(pixmapSize, mode);

            drawPrimitive(PE_PanelButtonTool, &optionPE, painter, widget);
            drawItemPixmap(painter, option->rect, Qt::AlignCenter, pixmap);
        }

    } else
        QCommonStyle::drawComplexControl(control, option, painter, widget);
}

void CustomStyle::drawControl(QStyle::ControlElement control, const QStyleOption *option,
                              QPainter *painter, const QWidget *widget) const {
    painter->save();

    if (control == CE_ToolBar) {
        QRect tbRect = option->rect;
        auto x1 = option->rect.x();
        auto x2 = x1 + option->rect.width();
        auto y = option->rect.y() + option->rect.height() - 1;

        painter->setPen(option->palette.midlight().color());
        painter->drawLine(x1, y, x2, y);
    } else if (control == CE_PushButtonLabel) {
        auto opt = (QStyleOptionButton *) option;
        QRect r = option->rect;

        auto textCol = option->palette.windowText().color();
        if (opt->features & QStyleOptionButton::DefaultButton) {
            auto f = painter->font();
            f.setBold(true);
            painter->setFont(f);
            textCol = option->palette.highlightedText().color();
        }
        if (!(opt->state & State_Enabled))
            textCol.setAlpha(127);
        painter->setPen(textCol);
        painter->drawText(r, Qt::AlignCenter, opt->text);
    } else if (control == CE_ScrollBarAddLine || control == CE_ScrollBarSubLine) {
        // skip, add page, sub page and slider are drawn over this areas
    } else if (control == CE_ScrollBarAddPage || control == CE_ScrollBarSubPage) {
        QRect r = option->rect;
        painter->setPen(Qt::NoPen);
        painter->setBrush(option->palette.base());
        painter->drawRect(r);
    } else if (control == CE_ScrollBarSlider) {
        QRect r = option->rect;
        painter->setPen(Qt::NoPen);
        painter->setBrush(option->palette.base());
        painter->drawRect(r); //fill background
        r.adjust(0, 2, -2, -2);

        painter->setBrush(UtilsGui::blend(option->palette.mid().color(),
                                          option->palette.base().color(), 0.5));
        painter->drawRect(r);
    } else if (control == CE_MenuItem) {
        auto menuOption = (QStyleOptionMenuItem *) option;
        auto r = option->rect;
        if (menuOption->menuItemType == QStyleOptionMenuItem::Normal) {
            painter->setPen(Qt::NoPen);
            QColor textCol;
            if (menuOption->state & State_Selected && menuOption->state & State_Enabled) {
                textCol = option->palette.highlightedText().color();
                painter->setBrush(option->palette.highlight());
            } else {
                textCol = option->palette.windowText().color();
                if (!(menuOption->state & State_Enabled))
                    textCol.setAlpha(127);
                painter->setBrush(option->palette.window());
            }

            painter->drawRect(r);
            painter->setPen(textCol);
            r.adjust(option->fontMetrics.height(), 0, 0, 0);
            painter->drawText(r, Qt::AlignVCenter | Qt::AlignLeft, menuOption->text);
        } else if (menuOption->menuItemType == QStyleOptionMenuItem::Separator) {
            auto x1 = r.x();
            auto x2 = x1 + r.width();
            auto y = r.y() + r.height() / 2;

            painter->setPen(option->palette.midlight().color());
            painter->drawLine(x1, y, x2, y);
        }
    } else
        QCommonStyle::drawControl(control, option, painter, widget);

    painter->restore();
}

int CustomStyle::pixelMetric(QStyle::PixelMetric metric, const QStyleOption *option, const QWidget *widget) const {
    auto lh = QApplication::fontMetrics().height();

    switch (metric) {
        case QStyle::PM_ToolTipLabelFrameWidth:
            return Utils::roundFrac(lh * 3, 5);
        case QStyle::PM_ToolBarItemSpacing:
            return Utils::roundFrac(lh, 3);
        case QStyle::PM_ScrollBarExtent:
            return Utils::roundFrac(lh, 2);
        case QStyle::PM_MenuPanelWidth:
            return 1;

        default:
            return QCommonStyle::pixelMetric(metric, option, widget);
    }
}

QSize CustomStyle::sizeFromContents(QStyle::ContentsType ct, const QStyleOption *opt,
                                    const QSize &contentsSize, const QWidget *widget) const {
    auto lh = opt->fontMetrics.height();

    if (ct == QStyle::CT_MenuItem) {
        auto menuOption = (QStyleOptionMenuItem *) opt;
        QSize rr = contentsSize;
        rr.setWidth(rr.width() + 2 * lh);
        if (menuOption->menuItemType != QStyleOptionMenuItem::Separator)
            rr.setHeight(rr.height() + Utils::roundFrac(2 * lh, 3));

        return rr;
    } else if (ct == QStyle::CT_PushButton) {
        auto wid = Utils::roundFrac(lh, 10);
        auto baseWidth = lh * 4 + 2 * wid;
        QSize rr = contentsSize;
        rr.setHeight(Utils::roundFrac(lh * 4, 3) + 2 * wid);
        if (rr.width() < baseWidth)
            rr.setWidth(baseWidth);
        else
            rr.setWidth(rr.width() + lh);

        return rr;
    } else if (ct == QStyle::CT_LineEdit) {
        auto wid = Utils::roundFrac(lh, 10);
        QSize rr = contentsSize;
        rr.setHeight(Utils::roundFrac(lh * 4, 3) + 2 * wid);
        return rr;
    } else
        return QCommonStyle::sizeFromContents(ct, opt, contentsSize, widget);
}

int CustomStyle::styleHint(QStyle::StyleHint sh, const QStyleOption *opt,
                           const QWidget *w, QStyleHintReturn *shret) const {
    switch (sh) {
        case QStyle::SH_MenuBar_MouseTracking:
        case QStyle::SH_Menu_MouseTracking:
            return 1;

        default:
            return QCommonStyle::styleHint(sh, opt, w, shret);
    }
}

QRect CustomStyle::subElementRect(QStyle::SubElement r, const QStyleOption *opt, const QWidget *widget) const {
    auto lh = opt->fontMetrics.height();
    if (r == QStyle::SE_LineEditContents) {
        auto wid = Utils::roundFrac(lh, 10);
        auto rr = QCommonStyle::subElementRect(r, opt, widget);
        return rr.adjusted(2 * wid, wid, -4 * wid, -2 * wid);
    } else
        return QCommonStyle::subElementRect(r, opt, widget);
}

QRect CustomStyle::subControlRect(QStyle::ComplexControl cc, const QStyleOptionComplex *opt,
                                  QStyle::SubControl sc, const QWidget *w) const {
    if (cc == CC_ScrollBar) {
        auto width = pixelMetric(PM_ScrollBarExtent, nullptr, nullptr);
        auto r = QCommonStyle::subControlRect(cc, opt, sc, w);

        // we don't draw AddLine and SubLine so we need to expand area of other control
        // elements of ScrollBar so we don't leave black areas behind
        if (sc != SC_ScrollBarAddLine && sc != SC_ScrollBarSubLine)
            r.adjust(0, -width, 0, width);

        return r;
    } else
        return QCommonStyle::subControlRect(cc, opt, sc, w);
}

