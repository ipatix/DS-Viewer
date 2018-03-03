#include <cassert>

#include "Image.h"

#define c6(n) uint128_t(uint64_t((n >> 2) & 0b111111))

// Color

Color::Color()
{
    r = 0;
    g = 0;
    b = 0;
}

Color::Color(uint8_t r, uint8_t g, uint8_t b)
{
    this->r = r;
    this->g = g;
    this->b = b;
}

Image::Image(size_t _height, size_t _width)
    : pixels(_height * _width), height(_height), width(_width)
{
}
