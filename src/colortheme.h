#ifndef SPACEDISPLAY_COLORTHEME_H
#define SPACEDISPLAY_COLORTHEME_H

#include <QIcon>
#include <QColor>
#include "resource_builder/resources.h"

class ColorTheme
{
public:
    enum class NativeStyle
    {
        NATIVE,
        NATIVE_DARK,
        NATIVE_LIGHT,
    };
    enum class CustomStyle
    {
        DARK,
        LIGHT
    };

    explicit ColorTheme(CustomStyle style);

    // for NATIVE style DARK or LIGHT will be determined from background color automatically
    ColorTheme(const QColor& _background, const QColor& _foreground, NativeStyle style = NativeStyle::NATIVE);

    QIcon createIcon(ResourceBuilder::ResourceId id);

    QColor tint(const QColor& src, float factor);

    static QColor blend(const QColor& foreground, const QColor& background, qreal bg_amount);

    QColor textFor(const QColor& bg);

    QColor background;
    QColor foreground;

    QColor iconEnabled;
    QColor iconDisabled;

    QColor viewTextLight;
    QColor viewTextDark;

    QColor viewDirFill;
    QColor viewFileFill;
    QColor viewUnknownFill;
    QColor viewFreeFill;

    QColor viewDirLine;
    QColor viewFileLine;
    QColor viewUnknownLine;
    QColor viewFreeLine;

private:
    void initStyle(const QColor& _background, const QColor& _foreground, CustomStyle style);

};


#endif //SPACEDISPLAY_COLORTHEME_H
