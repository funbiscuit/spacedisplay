
#ifndef CURVEDETECT_RESOURCES_H
#define CURVEDETECT_RESOURCES_H

#include <cstdint>
#include <vector>
#include <QPixmap>
#include <QImage>
#include <unordered_map>
#include "icon.h"
#include "resource_builder/resources.h"


class Resources
{
private:
    Resources(const Resources&);
    Resources& operator=(Resources&);
    Resources() = default;

public:

    static Resources& get();

    void init();

    //qt uses implicit sharing so its safe and efficient to pass pixmap by value
    QPixmap get_vector_pixmap(ResourceBuilder::ResourceId id, int width, const QColor& color, qreal strength=1.0);

//    std::vector<GLFWimage> get_app_icons();

    Icon* get_vector_icon(ResourceBuilder::ResourceId id, int width);


private:
    bool resourcesLoaded = false;

    //map key is icon type, value is icon object
    std::unordered_map<ResourceBuilder::ResourceId, std::unordered_map<int, Icon>> iconsMap;

    QImage tint(const QImage& src, QColor color, qreal strength=1.0);

//    uint8_t* get_vector_image(ResourceBuilder::ResourceId id, int width);

    void load_icons();

};

#endif //CURVEDETECT_RESOURCES_H
