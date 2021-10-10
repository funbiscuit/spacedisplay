
#include <cstdio>
#include <cstdint>
#include <chrono>
#include <iostream>
#include "utils.h"
#include "platformutils.h"

extern "C" {
#include <crc.h>
}

uint16_t Utils::strCrc16(const std::string &str) {
    return crc16((char *) str.c_str(), (uint16_t) str.length());
}

std::string Utils::formatSize(int64_t size) {
    const float KB = 1024.f;
    const float MB = KB * 1024.f;
    const float GB = MB * 1024.f;

    auto sz = (float) size;

    std::string sizeStr;

    if (sz > GB) {
        sz /= GB;
        sizeStr = strFormat("%0.1f GiB", sz);
    } else if (sz > MB) {
        sz /= MB;
        sizeStr = strFormat("%0.1f MiB", sz);
    } else if (sz > KB) {
        sz /= KB;
        sizeStr = strFormat("%0.1f KiB", sz);
    } else {
        sizeStr = strFormat("%0.1f B", sz);
    }

    return sizeStr;
}
