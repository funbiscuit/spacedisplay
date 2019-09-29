
#include <cstdio>
#include <cstdint>
#include <algorithm>
#include <chrono>
#include <iostream>
#include "utils.h"

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
    rgb[0] = rgb[0] + (255 - rgb[0]) * tint_factor;
    rgb[1] = rgb[1] + (255 - rgb[1]) * tint_factor;
    rgb[2] = rgb[2] + (255 - rgb[2]) * tint_factor;
}

bool in_array(const std::string &value, const std::vector<std::string> &array)
{
    return std::find(array.begin(), array.end(), value) != array.end();
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
        return dur;
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

std::string Utils::path_to_forward_slashes(std::string path)
{
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

std::string Utils::path_to_backslashes(std::string path)
{
    std::replace(path.begin(), path.end(), '/', '\\');
    return path;
}

std::string Utils::get_parent_path(std::string path)
{
    std::string parent_path;

    path=path_to_forward_slashes(path);

    auto len = path.length();

    if(path[len-1] == '/')
        path.resize(len-1);

    auto pos = path.find_last_of('/');
    if(pos!=std::string::npos)
        parent_path=path.substr(0,pos+1);
    else
        parent_path="";

    return parent_path;
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
        sizeStr=string_format("%0.1fGiB",sz);
    }
    else if(sz>MB)
    {
        sz/=MB;
        sizeStr=string_format("%0.1fMiB",sz);
    }
    else if(sz>KB)
    {
        sz/=KB;
        sizeStr=string_format("%0.1fKiB",sz);
    }
    else
    {
        sizeStr=string_format("%0.1fB",sz);
    }

    return sizeStr;
}