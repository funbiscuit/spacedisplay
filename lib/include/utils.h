
#ifndef SPACEDISPLAY_UTILS_H
#define SPACEDISPLAY_UTILS_H

#include <string>
#include <vector>
#include <memory>
#include <algorithm>

namespace Utils
{
    struct RectF{
        float x,y;
        float w,h;
    };
    struct RectI{
        int x,y;
        int w,h;
    };


    int _prof_meas(bool star, const char* message = nullptr, int threshold = 0);
    void tic();
    int toc(const char* message = nullptr, int threshold = 0);

    std::string format_size(int64_t size);

    //todo we don't need anything from C++14, just make_unique, so use this simple version with C++11
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
    template<typename T>
    std::unique_ptr<T[]> make_unique_arr(size_t N) {
        return std::unique_ptr<T[]>(new T[N]);
    }

     /**
      * Checks if given value is inside the array
      * @tparam T type of value
      * @param value
      * @param array
      * @return
      */
    template<typename T>
    bool in_array(const T& value, const std::vector<T>& array)
    {
        return std::find(array.begin(), array.end(), value) != array.end();
    }

    template <typename T>
    T clip(const T& n, const T& lower, const T& upper) {
        return std::max(lower, std::min(n, upper));
    }

    /**
     * Returns available mount points (that can be scanned)
     * @param availableMounts vector containing paths of safe mount points
     */
    void getMountPoints(std::vector<std::string>& availableMounts);
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

#endif //SPACEDISPLAY_UTILS_H
