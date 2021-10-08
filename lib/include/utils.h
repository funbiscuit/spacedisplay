
#ifndef SPACEDISPLAY_UTILS_H
#define SPACEDISPLAY_UTILS_H

#include <string>
#include <vector>
#include <memory>
#include <algorithm>

namespace Utils {
    struct RectI {
        int x, y;
        int w, h;
    };

    uint16_t strCrc16(const std::string &str);

    std::string formatSize(int64_t size);

    /**
     * Returns available mount points (that can be scanned)
     * @param availableMounts vector containing paths of safe mount points
     */
    void getMountPoints(std::vector<std::string> &availableMounts);

    //todo we don't need anything from C++14, just make_unique, so use this simple version with C++11
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args &&... args) {
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
    bool in_array(const T &value, const std::vector<T> &array) {
        return std::find(array.begin(), array.end(), value) != array.end();
    }

    template<typename T>
    T clip(const T &n, const T &lower, const T &upper) {
        return std::max(lower, std::min(n, upper));
    }

    /**
     * Round fraction `num/denom` to the nearest int.
     * Equivalent to int(round(double(num)/double(denom)))
     * @tparam T
     * @param num
     * @param denom
     * @return round(num/denom)
     */
    template<typename T>
    T roundFrac(T num, T denom) {
        static_assert(std::is_integral<T>::value, "Integral type is required");
        return (num + denom / 2) / denom;
    }

    template<typename ... Args>
    std::string strFormat(const char *format, Args ... args) {
        size_t size = snprintf(nullptr, 0, format, args ...) + 1; // Extra space for '\0'
        std::unique_ptr<char[]> buf(new char[size]);
        snprintf(buf.get(), size, format, args ...);
        return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }
}

#endif //SPACEDISPLAY_UTILS_H
