#include "colortheme.h"
#include "resources.h"

#include <utility>
#include <iostream>


ColorTheme::ColorTheme(CustomStyle _style)
{
    style = _style;
    if(_style == CustomStyle::DARK)
        initStyle(QColor(50,50,50), QColor(230,230,230), _style);
    else
        initStyle(QColor(250,250,250), QColor(35,35,35), CustomStyle::LIGHT);
}

ColorTheme::ColorTheme(const QColor& _background, const QColor& _foreground, NativeStyle _style)
{
    style = _style==NativeStyle::NATIVE_DARK ? CustomStyle::DARK : CustomStyle::LIGHT;

    //determine style from background
    if(_style == NativeStyle::NATIVE)
        style = _background.lightnessF() < 0.5f ? CustomStyle::DARK : CustomStyle::LIGHT;
    initStyle(_background,  _foreground, style);
}

void ColorTheme::initStyle(const QColor& _background, const QColor& _foreground, CustomStyle _style)
{
    style = _style;
    std::cout << "Using "<<(style==CustomStyle::DARK ? "dark" : "light")<<" style\n";

    background = _background;
    foreground = _foreground;
    viewTextLight = QColor(230, 230, 230);
    viewTextDark = QColor(60, 60, 60);

    if(style==CustomStyle::DARK)
    {
        viewDirFill = QColor(150, 130, 95);
        viewDirLine = QColor(120, 100, 75);

        viewFileFill = QColor(65, 85, 115);
        viewFileLine = QColor(50, 65, 90);

        viewFreeFill = QColor(73, 156, 84);
        viewFreeLine = QColor(50, 115, 60);

        viewUnknownFill = QColor(120, 120, 110);
        viewUnknownLine = QColor(105, 105, 95);

        iconEnabled = QColor(200, 200, 200);
    } else
    {
        viewDirFill = QColor(255, 222, 173);
        viewDirLine = QColor(204, 176, 138);

        viewFileFill = QColor(119, 158, 203);
        viewFileLine = QColor(94, 125, 160);

        viewFreeFill = QColor(119, 221, 119);
        viewFreeLine = QColor(95, 175, 95);

        viewUnknownFill = QColor(207, 207, 196);
        viewUnknownLine = QColor(165, 165, 157);

        iconEnabled = QColor(120, 120, 120);
    }

    iconDisabled = blend(iconEnabled, background, 0.7);

//    viewDirFill = QColor(234, 78, 34);
//    viewFileFill = QColor(78, 12, 46);
}

bool ColorTheme::isDark()
{
    return style == CustomStyle::DARK;
}

QIcon ColorTheme::createIcon(ResourceBuilder::ResId id)
{
    auto& r = Resources::get();

    QIcon icon(r.get_vector_pixmap(id,64, iconEnabled));
    icon.addPixmap(r.get_vector_pixmap(id,64, iconDisabled), QIcon::Disabled);

    return icon;
}

QColor ColorTheme::tint(const QColor& src, float factor)
{
    return ColorTheme::blend(src,background,factor);
}

QColor ColorTheme::blend(const QColor& foreground, const QColor& background, qreal bg_amount)
{
    return QColor::fromRgbF(
            foreground.redF()*(1.0-bg_amount) + background.redF()*bg_amount,
            foreground.greenF()*(1.0-bg_amount) + background.greenF()*bg_amount,
            foreground.blueF()*(1.0-bg_amount) + background.blueF()*bg_amount);
}

QColor ColorTheme::textFor(const QColor& bg)
{
    return bg.lightnessF()>0.5f ? viewTextDark : viewTextLight;
}
