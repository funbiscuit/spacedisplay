#include "customtheme.h"
#include <QApplication>

#include "resources.h"
#include "utils-gui.h"

CustomPalette::CustomPalette(Theme theme)
{
    palette = QApplication::palette();
    setTheme(theme);
}

const QPalette& CustomPalette::getPalette() const
{
    return palette;
}

bool CustomPalette::isDark()
{
    return UtilsGui::isDark(palette.window().color());
}

void CustomPalette::setTheme(Theme theme)
{
    if(theme == Theme::DARK)
    {
        palette.setColor(QPalette::Window, QColor(60,63,65));
        palette.setColor(QPalette::WindowText, QColor(187, 187, 187));
        palette.setColor(QPalette::Text, QColor(187,187,187));

        palette.setColor(QPalette::Midlight, QColor(81,81,81));
        palette.setColor(QPalette::Mid, QColor(92,97,100));
        palette.setColor(QPalette::Dark, QColor(175,177,179));
        palette.setColor(QPalette::Button, QColor(76,80,82));

        palette.setColor(QPalette::Highlight, QColor(75,110,175));
        palette.setColor(QPalette::HighlightedText, QColor(187,187,187));

        palette.setColor(QPalette::ToolTipBase, QColor(75,77,77));
        palette.setColor(QPalette::ToolTipText, QColor(191,191,191));

        viewDirFill = QColor(150, 130, 95);
        viewFileFill = QColor(65, 85, 115);
        viewAvailableFill = QColor(73, 156, 84);
        viewUnknownFill = QColor(120, 120, 110);
    } else
    {
        palette.setColor(QPalette::Window, QColor(242,242,242));
        palette.setColor(QPalette::WindowText, QColor(35,35,35));
        palette.setColor(QPalette::Text, QColor(0,0,0));

        palette.setColor(QPalette::Midlight, QColor(214,214,214));
        palette.setColor(QPalette::Mid, QColor(207,207,207));
        palette.setColor(QPalette::Dark, QColor(110,110,110));
        palette.setColor(QPalette::Button, QColor(255,255,255));

        palette.setColor(QPalette::Highlight, QColor(40,120,200));
        palette.setColor(QPalette::HighlightedText, QColor(255,255,255));

        palette.setColor(QPalette::ToolTipBase, QColor(247,247,247));
        palette.setColor(QPalette::ToolTipText, QColor(35,35,35));

        viewDirFill = QColor(255, 222, 173);
        viewFileFill = QColor(119, 158, 203);
        viewAvailableFill = QColor(119, 221, 119);
        viewUnknownFill = QColor(207, 207, 196);
    }
}

QColor CustomPalette::bgBlend(const QColor& src, double factor)
{
    return UtilsGui::blend(src, palette.window().color(), factor);
}

QColor CustomPalette::getTextColorFor(const QColor& bg)
{
    return bg.lightnessF()>0.5f ? QColor(60, 60, 60) : QColor(230, 230, 230);
}

QIcon CustomPalette::createIcon(ResourceBuilder::ResId id)
{
    auto& r = Resources::get();

    auto color = palette.dark().color();

    QIcon icon(r.get_vector_pixmap(id,64, color));
    icon.addPixmap(r.get_vector_pixmap(id,64, bgBlend(color, 0.7)), QIcon::Disabled);

    return icon;
}

QColor CustomPalette::getViewDirFill()
{
    return viewDirFill;
}

QColor CustomPalette::getViewDirLine()
{
    return viewDirFill.darker(125);
}

QColor CustomPalette::getViewFileFill()
{
    return viewFileFill;
}

QColor CustomPalette::getViewFileLine()
{
    return viewFileFill.darker(125);
}

QColor CustomPalette::getViewAvailableFill()
{
    return viewAvailableFill;
}

QColor CustomPalette::getViewAvailableLine()
{
    return viewAvailableFill.darker(125);
}

QColor CustomPalette::getViewUnknownFill()
{
    return viewUnknownFill;
}

QColor CustomPalette::getViewUnknownLine()
{
    return viewUnknownFill.darker(125);
}
