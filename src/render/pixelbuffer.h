#include <deque>
#include <stdint.h>

class PixelBuffer {
public:
    struct Pixel {
        uint32_t x, y;
        uint32_t color;
    };
    void append(Pixel && p) {
        buffer.emplace_back(std::move(p));
    }

    void erase_old(unsigned max) {
        if (buffer.size() > max + 1024*4)
            buffer.erase(buffer.begin(), buffer.end() - max);
    }
    std::deque<Pixel> buffer;
};
