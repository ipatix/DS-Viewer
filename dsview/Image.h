#pragma once

#include <cstdint>
#include <vector>
#include <cstddef>
#include <cassert>

struct Color
{
    Color() {}
    Color(uint8_t r, uint8_t g, uint8_t b) {
        this->r = r;
        this->g = g;
        this->b = b;
    }
    uint8_t r, g, b;
};

static_assert(sizeof(struct Color) == 3, "Size of one pixel must be 3 bytes");

class Image
{
    public:
        Image(size_t _height, size_t _width)
            : pixels(_height * _width), height(_height), width(_width) { }
        Color& operator() (size_t y, size_t x) {
            assert(y < height);
            assert(x < width);
            return pixels[width * y + x];
        }

        size_t Width() const { return width; };
        size_t Height() const { return height; };

        void *getData() { return pixels.data(); }

    private:
        std::vector<Color> pixels;
        const size_t height;
        const size_t width;
};