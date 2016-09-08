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

void BlockToCol(Color& tl, Color& bl, Color& tr, Color& br, uint128_t data)
{
    static const uint8_t color_mask = 0b111111; // 6 bit color
    uint8_t r, g, b;
    // top left
    r = (uint8_t(data) & color_mask) << 2;
    data >>= 6;
    g = (uint8_t(data) & color_mask) << 2;
    data >>= 6;
    b = (uint8_t(data) & color_mask) << 2;
    data >>= 6;
    tl = Color(r, g, b);
    // bot left
    r = (uint8_t(data) & color_mask) << 2;
    data >>= 6;
    g = (uint8_t(data) & color_mask) << 2;
    data >>= 6;
    b = (uint8_t(data) & color_mask) << 2;
    data >>= 6;
    bl = Color(r, g, b);
    // top right
    r = (uint8_t(data) & color_mask) << 2;
    data >>= 6;
    g = (uint8_t(data) & color_mask) << 2;
    data >>= 6;
    b = (uint8_t(data) & color_mask) << 2;
    data >>= 6;
    tr = Color(r, g, b);
    // bot right
    r = (uint8_t(data) & color_mask) << 2;
    data >>= 6;
    g = (uint8_t(data) & color_mask) << 2;
    data >>= 6;
    b = (uint8_t(data) & color_mask) << 2;
    data >>= 6;
    br = Color(r, g, b);
}

uint128_t ColToBlock(const Color& tl, const Color& bl, const Color& tr, const Color& br)
{
	uint128_t c = c6(tl.r);
	c |= (c6(tl.g) << uint128_t(6));
	c |= (c6(tl.b) << uint128_t(12));
	c |= (c6(bl.r) << uint128_t(18));
	c |= (c6(bl.g) << uint128_t(24));
	c |= (c6(bl.b) << uint128_t(30));
	c |= (c6(tr.r) << uint128_t(36));
	c |= (c6(tr.g) << uint128_t(42));
	c |= (c6(tr.b) << uint128_t(48));
	c |= (c6(br.r) << uint128_t(54)); 
	c |= (c6(br.g) << uint128_t(60));
	c |= (c6(br.b) << uint128_t(66));

    return c;
}

Image::Image(size_t _height, size_t _width)
    : pixels(_height * _width), height(_height), width(_width)
{
}

Color& Image::operator() (size_t y, size_t x)
{
    assert(y < height);
    assert(x < width);
    return pixels[width * y + x];
}

size_t Image::Width() const
{
    return width;
}

size_t Image::Height() const
{
    return height;
}
