#ifndef SPACEDISPLAY_COLORTHEME_H
#define SPACEDISPLAY_COLORTHEME_H

#include <QColor>

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

    QColor tint(QColor src, float factor);

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
