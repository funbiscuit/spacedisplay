
#include <QPainter>
#include <QGraphicsEffect>

#include "resources.h"
#include "resource_builder/resources.h"

#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsObject>
#include <QtSvg/qsvgrenderer.h>
#include <iostream>
#include <memory>
#include <cmath>


Resources &Resources::get() {
    static Resources instance;
    return instance;
}

QImage Resources::tint(const QImage &src, const QColor &color, qreal strength) {
    // FIXME tint result is not very good if source is complete black and target is complete wight
    if (src.isNull()) return QImage();
    QGraphicsScene scene;
    QGraphicsPixmapItem item;
    item.setPixmap(QPixmap::fromImage(src));
    QGraphicsColorizeEffect effect;
    effect.setColor(color);
    effect.setStrength(strength);
    item.setGraphicsEffect(&effect);
    scene.addItem(&item);
    QImage res(src);
    QPainter ptr(&res);
    scene.render(&ptr, QRectF(), src.rect());
    return res;
}

QPixmap Resources::get_vector_pixmap(ResourceBuilder::ResId id, int width) {
    return get_vector_pixmap(id, width, QColor(Qt::white), 0.0);
}

QPixmap Resources::get_vector_pixmap(ResourceBuilder::ResId id, int width, const QColor &color, qreal strength) {
    auto data = ResourceBuilder::get_resource_data(id);
    auto size = ResourceBuilder::get_resource_size(id);

    QByteArray svgData = QByteArray::fromRawData((const char *) data, size);
    QSvgRenderer qSvgRenderer(svgData);
    auto sz = QSizeF(qSvgRenderer.defaultSize());

    auto scale = double(width) / sz.width();
    auto height = (int) std::round(sz.height() * scale);

    QImage img(width, height, QImage::Format_RGBA8888);

    img.fill(Qt::transparent);
    QPainter painter_pix(&img);
    qSvgRenderer.render(&painter_pix);

    return QPixmap::fromImage(tint(img, color, strength));
}
