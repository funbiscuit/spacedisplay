#ifndef SPACEDISPLAY_CUSTOMTHEME_H
#define SPACEDISPLAY_CUSTOMTHEME_H

#include <QPalette>
#include <QColor>
#include <QIcon>

#include "resource_builder/resources.h"

class CustomPalette {
public:
    enum class Theme {
        DARK,
        LIGHT
    };

    explicit CustomPalette(Theme theme = Theme::LIGHT);

    void setTheme(Theme theme);

    bool isDark();

    const QPalette &getPalette() const;

    QColor getViewDirFill();

    QColor getViewDirLine();

    QColor getViewFileFill();

    QColor getViewFileLine();

    QColor getViewAvailableFill();

    QColor getViewAvailableLine();

    QColor getViewUnknownFill();

    QColor getViewUnknownLine();

    QColor bgBlend(const QColor &src, double factor);

    static QColor getTextColorFor(const QColor &bg);

    QIcon createIcon(ResourceBuilder::ResId id);

private:
    QPalette palette;

    QColor viewDirFill;
    QColor viewFileFill;
    QColor viewUnknownFill;
    QColor viewAvailableFill;
};

#endif //SPACEDISPLAY_CUSTOMTHEME_H
