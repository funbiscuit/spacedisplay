
#ifndef CURVEDETECT_RESOURCES_H
#define CURVEDETECT_RESOURCES_H

#include <cstdint>
#include <vector>
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

//    std::vector<GLFWimage> get_app_icons();

    Icon* get_vector_icon(ResourceBuilder::ResourceId id, int width);


private:
    bool resourcesLoaded = false;

    //map key is icon type, value is icon object
    std::unordered_map<ResourceBuilder::ResourceId, std::unordered_map<int, Icon>> iconsMap;

    uint8_t* get_vector_image(ResourceBuilder::ResourceId id, int width);

    void load_icons();

};

#endif //CURVEDETECT_RESOURCES_H
