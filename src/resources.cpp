
#include <QPainter>
#include <QGraphicsEffect>

#include "resources.h"
#include "resource_builder/resources.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsObject>
#include <iostream>

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

Resources& Resources::get()
{
    static Resources instance;
    return instance;
}

QImage Resources::tint(const QImage& src, const QColor& color, qreal strength)
{
    // FIXME tint result is not very good if source is complete black and target is complete wight
    if(src.isNull()) return QImage();
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
    scene.render(&ptr, QRectF(), src.rect() );
    return res;
}

QPixmap Resources::get_vector_pixmap(ResourceBuilder::ResourceId id, int width)
{
    return get_vector_pixmap(id,width,QColor(Qt::white),0.0);
}

QPixmap Resources::get_vector_pixmap(ResourceBuilder::ResourceId id, int width, const QColor& color, qreal strength)
{
    auto data=ResourceBuilder::get_resource_data(id);
    auto size=ResourceBuilder::get_resource_size(id);

    char* data_cp = new char[size];
    memcpy(data_cp, data, size);

    NSVGimage *svgImage;
    NSVGrasterizer *rast;
    uint8_t* img;

    svgImage = nsvgParse(data_cp, "px", 96.0f);
    delete[](data_cp);
    if (svgImage == nullptr) {
        return QPixmap(0,0);
    }
    float w0 = svgImage->width;
    float scale = float(width)/w0;
    auto height = (int)std::round(svgImage->height*scale);

    rast = nsvgCreateRasterizer();

    img = new uint8_t[width*height*4];

    printf("rasterizing vector image at %d x %d\n", width, height);
    nsvgRasterize(rast, svgImage, 0, 0, scale, img, width, height, width * 4);

    nsvgDeleteRasterizer(rast);
    nsvgDelete(svgImage);
    QImage myImage(img, width, height, width*4, QImage::Format_RGBA8888);
    QPixmap myPixmap = QPixmap::fromImage(tint(myImage, color, strength));

    delete[](img);

    return myPixmap;
}
