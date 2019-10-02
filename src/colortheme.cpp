#include "colortheme.h"

#include <utility>
#include <iostream>


ColorTheme::ColorTheme(CustomStyle style)
{
    if(style == CustomStyle::DARK)
        initStyle(QColor(50,50,50), QColor(230,230,230), style);
    else
        initStyle(QColor(250,250,250), QColor(35,35,35), CustomStyle::LIGHT);
}

ColorTheme::ColorTheme(const QColor& _background, const QColor& _foreground, NativeStyle style)
{
    auto customStyle = style==NativeStyle::NATIVE_DARK ? CustomStyle::DARK : CustomStyle::LIGHT;

    //determine style from background
    if(style == NativeStyle::NATIVE)
        customStyle = _background.lightnessF() < 0.5f ? CustomStyle::DARK : CustomStyle::LIGHT;
    initStyle(_background,  _foreground, customStyle);
}

void ColorTheme::initStyle(const QColor& _background, const QColor& _foreground, CustomStyle style)
{
    background = _background;
    foreground = _foreground;
    viewTextLight = QColor(230, 230, 230);
    viewTextDark = QColor(60, 60, 60);

    viewDirFill = QColor(255, 222, 173);
    viewFileFill = QColor(119, 158, 203);
    viewUnknownFill = QColor(207, 207, 196);
    viewFreeFill = QColor(119, 221, 119);

    viewDirLine = QColor(204, 176, 138);
    viewFileLine = QColor(94, 125, 160);
    viewUnknownLine = QColor(165, 165, 157);
    viewFreeLine = QColor(95, 175, 95);


//    viewDirFill = QColor(234, 78, 34);
//    viewFileFill = QColor(78, 12, 46);
}

QColor ColorTheme::tint(QColor src, float factor)
{
    qreal h, s, l;
    src.getHslF(&h,&s,&l);
    l = l + (1-l)*factor;
    src.setHslF(h, s, l);

    return src;
}

QColor ColorTheme::textFor(const QColor& bg)
{
    return bg.lightnessF()>0.5f ? viewTextDark : viewTextLight;
}
