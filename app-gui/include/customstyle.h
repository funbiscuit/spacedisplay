#ifndef SPACEDISPLAY_CUSTOMSTYLE_H
#define SPACEDISPLAY_CUSTOMSTYLE_H

#include <QCommonStyle>

class CustomStyle : public QCommonStyle {
Q_OBJECT

public:
    CustomStyle() = default;

    ~CustomStyle() override = default;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget) const override;

    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) const override;

    void drawControl(ControlElement control, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const override;

    int pixelMetric(PixelMetric metric, const QStyleOption *option,
                    const QWidget *widget) const override;

    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt,
                           const QSize &contentsSize, const QWidget *widget) const override;

    QRect
    subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *w) const override;

    QRect subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget) const override;

    int styleHint(StyleHint sh, const QStyleOption *opt,
                  const QWidget *w, QStyleHintReturn *shret) const override;

};


#endif //SPACEDISPLAY_CUSTOMSTYLE_H
