#pragma once

#include <cstdint>
#include <vector>
#include <cstddef>
#include <cassert>

#include "uint128_t.h"

struct Color
{
    Color();
    Color(uint8_t r, uint8_t g, uint8_t b);
    uint8_t r, g, b;
};

void BlockToCol(Color& tl, Color& bl, Color& tr, Color& br, uint128_t data);
uint128_t ColToBlock(const Color& tl, const Color& bl, const Color& tr, const Color& br);

class Image
{
    public:
        Image(size_t _height, size_t _width);
        Color& operator() (size_t y, size_t x) {
            assert(y < height);
            assert(x < width);
            return pixels[width * y + x];
        }

        size_t Width() const { return width; };
        size_t Height() const { return height; };

    private:
        std::vector<Color> pixels;
        const size_t height;
        const size_t width;
};
