
#include "resources.h"
#include "resource_builder/resources.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

Resources& Resources::get()
{
    static Resources instance;
    return instance;
}

void Resources::init()
{
}

Icon* Resources::get_vector_icon(ResourceBuilder::ResourceId id, int width)
{
    auto it1 = iconsMap.find(id);
    if(it1 != iconsMap.end())
    {
        auto it2 = it1->second.find(width);
        if(it2!= it1->second.end())
            return &(it2->second);
    }

    auto* pixels = get_vector_image(id, width);

    if(pixels == nullptr)
        return nullptr;

    if(it1 != iconsMap.end())
    {
        auto it2 = it1->second.insert(std::pair<int, Icon>(width, Icon(pixels, width, width))).first;
        return &(it2->second);
    }

    it1 = iconsMap.insert(std::pair<ResourceBuilder::ResourceId, std::unordered_map<int, Icon>>(id, std::unordered_map<int, Icon>())).first;

    auto it2 = it1->second.insert(std::pair<int, Icon>(width, Icon(pixels, width, width))).first;
    return &(it2->second);
}

uint8_t* Resources::get_vector_image(ResourceBuilder::ResourceId id, int width)
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
        return nullptr;
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
    return img;
}

//std::vector<GLFWimage> Resources::get_app_icons()
//{
//    std::vector<GLFWimage> images;
//    for(int w0=16;w0<=256;w0*=2)
//    {
//        images.emplace_back();
//        images.back().pixels = get_vector_image(ResourceBuilder::RES___ICONS_SVG_APPICON_SVG, w0);
//        images.back().width = w0;
//        images.back().height = w0;
//    }
//
//    return images;
//}
