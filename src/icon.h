
#ifndef SPACEDISPLAY_ICON_H
#define SPACEDISPLAY_ICON_H

#include <cstdint>

class Icon {
public:
    Icon(const uint8_t* pixels, int width, int height);

//    unsigned int textureId;
    int width;
    int height;
//    ImVec2 uv0;
//    ImVec2 uv1;

};

#endif //SPACEDISPLAY_ICON_H
