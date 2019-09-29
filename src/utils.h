
#ifndef SPACEDISPLAY_UTILS_H
#define SPACEDISPLAY_UTILS_H

#include <string>
#include <vector>
#include <memory>

namespace Utils
{
    struct Rect{
        float x,y;
        float w,h;
    };


    int _prof_meas(bool star, const char* message = nullptr);
    void tic();
    int toc(const char* message = nullptr);

    std::string path_to_forward_slashes(std::string path);
    std::string path_to_backslashes(std::string path);

    std::string get_parent_path(std::string path);
    std::string format_size(int64_t size);

}

void hex_to_rgb(int hex_color, float (&rgb)[3]);
void hex_to_rgbi(int hex_color, int (&rgb)[3]);

/**
 * Convert hex color to rgb and lighten it
 * newRGB = currentRGB + (1.f - currentRGB) * tint_factor
 * @param hex_color - 0xrrggbb color value
 * @param rgb       - output of rgb values
 * @param tint_factor     - how much to lighten (0 to 1)
 */
void hex_to_rgb_tint(int hex_color, float (&rgb)[3], float tint_factor);
void hex_to_rgbi_tint(int hex_color, int (&rgb)[3], float tint_factor);
bool in_array(const std::string &value, const std::vector<std::string> &array);

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    return string_format(format.c_str(), args ...);
}

template<typename ... Args>
std::string string_format( const char* format, Args ... args )
{
    size_t size = snprintf( nullptr, 0, format, args ... ) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf( new char[ size ] );
    snprintf( buf.get(), size, format, args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

void open_folder_in_file_manager(const char* folder_path);
void show_file_in_file_manager(const char* file_path);

/**
 * System specific utils
 */

#ifdef _WIN32

std::wstring str2wstr(std::string const &str);
std::string wstr2str(std::wstring const &str);

#endif


#endif //SPACEDISPLAY_UTILS_H
