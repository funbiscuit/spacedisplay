
#include <cstdio>
#include <cstdint>
#include <chrono>
#include <iostream>
#include "utils.h"

#include <portable-file-dialogs.h>

void hex_to_rgb(int hex_color, float (&rgb)[3])
{
//    printf("Input: %06X\n", hex_color);
    
    rgb[0] = ((hex_color >> 16) & 0xFF) / 255.f;  // Extract the RR byte
    rgb[1] = ((hex_color >> 8) & 0xFF) / 255.f;   // Extract the GG byte
    rgb[2] = ((hex_color) & 0xFF) / 255.f;        // Extract the BB byte
}

void hex_to_rgbi(int hex_color, int (&rgb)[3])
{
//    printf("Input: %06X\n", hex_color);

    rgb[0] = ((hex_color >> 16) & 0xFF);  // Extract the RR byte
    rgb[1] = ((hex_color >> 8) & 0xFF);   // Extract the GG byte
    rgb[2] = ((hex_color) & 0xFF);        // Extract the BB byte
}

void hex_to_rgb_tint(int hex_color, float (&rgb)[3], float tint_factor)
{
    hex_to_rgb(hex_color, rgb);
    rgb[0] = rgb[0] + (1.f - rgb[0]) * tint_factor;
    rgb[1] = rgb[1] + (1.f - rgb[1]) * tint_factor;
    rgb[2] = rgb[2] + (1.f - rgb[2]) * tint_factor;
}

void hex_to_rgbi_tint(int hex_color, int (&rgb)[3], float tint_factor)
{
    //TODO
    hex_to_rgbi(hex_color, rgb);
    for(int & i : rgb)
        i = i + int(float(255 - i) * tint_factor);
}

int Utils::_prof_meas(bool start, const char* message)
{
    static auto begin = std::chrono::high_resolution_clock::now();

    if(start)
        begin = std::chrono::high_resolution_clock::now();
    else
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count();
        if(message)
            std::cout << message << ": "<<dur<<"ms\n";
        else
            std::cout << "toc: "<<dur<<"ms\n";
        return int(dur);
    }
    return 0;
}

void Utils::tic()
{
    Utils::_prof_meas(true);
}

int Utils::toc(const char* message)
{
    return Utils::_prof_meas(false, message);
}

std::string Utils::format_size(int64_t size)
{
    const float KB = 1024.f;
    const float MB = KB*1024.f;
    const float GB = MB*1024.f;

    auto sz=(float)size;

    std::string sizeStr;

    if(sz>GB)
    {
        sz/=GB;
        sizeStr=string_format("%0.1f GiB",sz);
    }
    else if(sz>MB)
    {
        sz/=MB;
        sizeStr=string_format("%0.1f MiB",sz);
    }
    else if(sz>KB)
    {
        sz/=KB;
        sizeStr=string_format("%0.1f KiB",sz);
    }
    else
    {
        sizeStr=string_format("%0.1f B",sz);
    }

    return sizeStr;
}

std::string Utils::select_folder(const std::string& title)
{
    // simple wrapper so we need less memory to compile
    return pfd::select_folder(title).result();
}
