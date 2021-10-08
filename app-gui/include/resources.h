
#ifndef CURVEDETECT_RESOURCES_H
#define CURVEDETECT_RESOURCES_H

#include <cstdint>
#include <vector>
#include <QPixmap>
#include <QImage>
#include "resource_builder/resources.h"


class Resources {
private:
    Resources() = default;

public:
    Resources(const Resources &) = delete;

    Resources &operator=(Resources &) = delete;

    static Resources &get();

    //qt uses implicit sharing so its safe and efficient to pass pixmap by value
    QPixmap get_vector_pixmap(ResourceBuilder::ResId id, int width, const QColor &color, qreal strength = 1.0);

    QPixmap get_vector_pixmap(ResourceBuilder::ResId id, int width);

private:

    QImage tint(const QImage &src, const QColor &color, qreal strength = 1.0);

};

#endif //CURVEDETECT_RESOURCES_H
