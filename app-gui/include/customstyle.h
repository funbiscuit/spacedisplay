#ifndef SPACEDISPLAY_CUSTOMSTYLE_H
#define SPACEDISPLAY_CUSTOMSTYLE_H

#include <QProxyStyle>

class CustomStyle : public QProxyStyle
{
Q_OBJECT

public:

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget) const override;

    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) const override;

    void drawControl(ControlElement control, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const override;

    int pixelMetric(PixelMetric metric, const QStyleOption *option,
                    const QWidget *widget) const override;
};



#endif //SPACEDISPLAY_CUSTOMSTYLE_H
