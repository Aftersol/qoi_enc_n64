/*

    colorconv.h - Color conversion utilities for N64 QOI encoder

    Code licensed under MIT License

    Copyright (c) 2026 Aftersol

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

*/

#ifndef COLORCONV_H
#define COLORCONV_H

#if __cplusplus
extern "C" {
#endif

#include <stdint.h>

/// @brief Reads an 16-bit unsigned big endian integer number
/// @param val value of a 16-bit unsigned integer in big endian 
/// @return converted value of the unsigned big endian value passed into read_be_u16 in their endianness of their machine
uint16_t read_be_u16(uint16_t val)
{
    uint8_t* val_ptr = (uint8_t*)&val;
    return (uint16_t)((val_ptr[1] << 0) | (val_ptr[0] << 8));
}

/// @brief Convert N64 16-bit (5R-5G-5B-1A) to 0xRRGGBBAA (32-bit).
/// @param px16 The 16-bit pixel value to convert
/// @return The converted 32-bit RGBA pixel value
inline uint32_t n64_color16_to_rgba32(uint16_t px16)
{
    int r = (read_be_u16(px16) >> 11) & 0x1F;
    int g = (read_be_u16(px16) >>  6) & 0x1F;
    int b = (read_be_u16(px16) >>  1) & 0x1F;
    int a = (read_be_u16(px16) >>  0) & 0x01;

    // expand to 8 bit
    r = (r << 3) | (r >> 2);
    g = (g << 3) | (g >> 2);
    b = (b << 3) | (b >> 2);
    a = a * 255;

    return (r << 24) | (g << 16) | (b << 8) | a;
}

#if __cplusplus
}
#endif

#endif // COLORCONV_H