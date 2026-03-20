/*

    qoi_enc_n64.h - QOI Encoder for N64

    This header file is modified from Simplified QOI Encoder code
    for the Nintendo 64. It provides functions to encode the N64 framebuffer
    into the QOI format. The encoder is designed to be efficient and suitable
    for the constraints of the N64 hardware.

    Simplified QOI Encoder: https://github.com/Aftersol/Simplified-QOI-Codec

    QOI Format Website: https://qoiformat.org/
    QOI Specification: https://qoiformat.org/qoi-specification.pdf

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

#ifndef QOI_ENC_N64_H
#define QOI_ENC_N64_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "colorconv.h"

/* QOI OPCODES */

#define QOI_TAG      0xC0
#define QOI_TAG_MASK 0x3F

#define QOI_OP_RGB   0xFE
#define QOI_OP_RGBA  0xFF

#define QOI_OP_INDEX 0x00
#define QOI_OP_DIFF  0x40
#define QOI_OP_LUMA  0x80
#define QOI_OP_RUN   0xC0

enum qoi_pixel_color {QOI_RED, QOI_GREEN, QOI_BLUE, QOI_ALPHA};
enum qoi_channels {QOI_WHITESPACE = 3, QOI_TRANSPARENT = 4};
enum qoi_colorspace {QOI_SRGB, QOI_LINEAR};

/// @brief QOI magic number
static const uint8_t QOI_MAGIC[4] = {'q', 'o', 'i', 'f'};

/// @brief QOI end of file padding bytes
static const uint8_t QOI_PADDING[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

/// @brief QOI descriptor as read by the header of a QOI file
typedef struct
{
    uint32_t width; /* in big endian */
    uint32_t height; /* in big endian */
    uint8_t channels;
    uint8_t colorspace;
} qoi_desc_t;

/// @brief pixel values
typedef union
{
    struct {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t alpha;
    };

    uint8_t channels[4];
    uint32_t concatenated_pixel_values;
} qoi_pixel_t;


/// @brief QOI encoder structure
typedef struct
{
    /*
        A running array[64] (zero-initialized) of previously seen pixel
        values is maintained by the encoder and decoder. Each pixel that is
        seen by the encoder and decoder is put into this array at the
        position formed by a hash function of the color value. 
    */
    qoi_pixel_t pix_buffer[64];
    qoi_pixel_t prev_pixel;

    uint8_t buffer0[4096];

    uint16_t buffer_offset;

    size_t pixel_offset, len;

    uint8_t* data;
    size_t pixels_written;

    uint8_t run;
} qoi_enc_t;


/* Machine specific code */

static inline uint32_t qoi_get_be32(uint32_t value);
static inline uint32_t qoi_to_be32(uint32_t value);

/* Pixel related code */

void qoi_set_pixel_rgb(qoi_pixel_t* pixel, uint8_t red, uint8_t green, uint8_t blue);
void qoi_set_pixel_rgba(qoi_pixel_t* pixel, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);

void qoi_initalize_pixel(qoi_pixel_t* pixel);
static bool qoi_cmp_pixel(qoi_pixel_t pixel1, qoi_pixel_t pixel2, const uint8_t channels);
static inline int32_t qoi_get_index_position(qoi_pixel_t pixel);

/* QOI descriptor functions */

bool qoi_desc_init(qoi_desc_t *desc);

void qoi_set_dimensions(qoi_desc_t *desc, uint32_t width, uint32_t height);
void qoi_set_channels(qoi_desc_t* desc, uint8_t channels);
void qoi_set_colorspace(qoi_desc_t* desc, uint8_t colorspace);

void write_qoi_header(qoi_desc_t *desc, void* dest);
bool read_qoi_header(qoi_desc_t *desc, void* data);

/* QOI encoder functions */

bool qoi_enc_init(qoi_desc_t* desc, qoi_enc_t* enc);
bool qoi_enc_done(qoi_enc_t* enc);

void qoi_encode_chunk(qoi_desc_t *desc, qoi_enc_t *enc, void *qoi_pixel_bytes);

static inline void qoi_enc_rgb(qoi_enc_t *enc, qoi_pixel_t px);
static inline void qoi_enc_rgba(qoi_enc_t *enc, qoi_pixel_t px);

static inline void qoi_enc_index(qoi_enc_t *enc, uint8_t index_pos);
static inline void qoi_enc_diff(qoi_enc_t *enc, uint8_t red_diff, uint8_t green_diff, uint8_t blue_diff);
static inline void qoi_enc_luma(qoi_enc_t *enc, uint8_t green_diff, uint8_t dr_dg, uint8_t db_dg);
static inline void qoi_enc_run(qoi_enc_t *enc);

/// @brief Extract a 32-bit big endian integer regardless of endianness
/// @param value The value to extract the big endian integer from
/// @return The value in big endian format
static inline uint32_t qoi_get_be32(uint32_t value)
{
    uint8_t* bytes = (uint8_t*)&value;
    uint32_t be_value = (uint32_t) (
            (bytes[0] << 24) |
            (bytes[1] << 16) |
            (bytes[2] << 8) |
            (bytes[3])
        );
    
    return be_value;
}

/// @brief Write a 32-bit big endian integer regardless of endianness
/// @param value The value to convert to big endian format
/// @return The value in big endian format
static inline uint32_t qoi_to_be32(uint32_t value)
{
    uint8_t bytes[4];

    bytes[0] = (value >> 24);
    bytes[1] = (value >> 16);
    bytes[2] = (value >> 8);
    bytes[3] = (value);
    
    return *((uint32_t*)bytes);
}

/// @brief Compares two pixels for the same color
/// @param pixel1 The first pixel to compare
/// @param pixel2 The second pixel to compare
/// @param channels The amount of channels to compare for the pixels (3 for RGB, 4 for RGBA)
/// @return If the two pixels are the same color
static bool qoi_cmp_pixel(qoi_pixel_t pixel1, qoi_pixel_t pixel2, const uint8_t channels)
{
    if (channels < 4) /* RGB pixels have three channels; RGBA pixels have four channels for the alpha channel */
    {
        /* O2 optimization will mask out these alpha values using OR instruction so it compares only the colors of a pixel */
        pixel1.alpha = 255;
        pixel2.alpha = 255;
    }

    return pixel1.concatenated_pixel_values == pixel2.concatenated_pixel_values; /* compare pixels */
}

/// @brief Sets the RGB pixel by a certain pixel value
/// @param pixel The pixel to set the RGB values for
/// @param red The red value to set for the pixel
/// @param green The green value to set for the pixel
/// @param blue The blue value to set for the pixel
inline void qoi_set_pixel_rgb(qoi_pixel_t* pixel, uint8_t red, uint8_t green, uint8_t blue)
{
    pixel->red = red;
    pixel->green = green;
    pixel->blue = blue;
}

/// @brief Sets the RGBA pixel by a certain pixel value including an transparency alpha value
/// @param pixel The pixel to set the RGBA values for
/// @param red The red value to set for the pixel
/// @param green The green value to set for the pixel
/// @param blue The blue value to set for the pixel
/// @param alpha The alpha value to set for the pixel
inline void qoi_set_pixel_rgba(qoi_pixel_t* pixel, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    pixel->red = red;
    pixel->green = green;
    pixel->blue = blue;
    pixel->alpha = alpha;
}

/// @brief Initalizes the pixels to the default state
/// @param pixel The pixel to initialize
void qoi_initalize_pixel(qoi_pixel_t* pixel)
{
    if (pixel == NULL) return;
    *pixel = (qoi_pixel_t){0};
}

/// @brief Hashing function for pixels: up to 64 possible hash values
/// @param pixel The pixel to get the index position for
/// @return The index position of the pixel in the buffer
static inline int32_t qoi_get_index_position(qoi_pixel_t pixel)
{
    return (pixel.red * 3 + pixel.green * 5 + pixel.blue * 7 + pixel.alpha * 11) % 64;
}

/// @brief Initalize the QOI desciptor to the default values
/// @param desc QOI descriptor to initialize
/// @return If the descriptor initialized successfully
bool qoi_desc_init(qoi_desc_t *desc)
{
    if (desc == NULL) return false;

    desc->width = 0;
    desc->height = 0;
    desc->channels = 0;
    desc->colorspace = 0;

    return true;
};

/// @brief Sets the image dimensions of an image for QOI descriptor
/// @param desc QOI descriptor to set the dimensions for
/// @param width The width to set for the image in the QOI descriptor
/// @param height The height to set for the image in the QOI descriptor
inline void qoi_set_dimensions(qoi_desc_t* desc, uint32_t width, uint32_t height)
{
    desc->width = width;
    desc->height = height;
}

/// @brief Sets the amount of channels of an image for QOI descriptor
/// @param desc QOI descriptor to set the channels for
/// @param channels The amount of channels to set for the image in the QOI descriptor
inline void qoi_set_channels(qoi_desc_t* desc, uint8_t channels)
{
    desc->channels = channels;
}

/// @brief Sets the colorspace of an image for QOI descriptor
/// @param desc QOI descriptor to set the colorspace for
/// @param colorspace The colorspace to set for the image in the QOI descriptor
inline void qoi_set_colorspace(qoi_desc_t *desc, uint8_t colorspace)
{
    desc->colorspace = colorspace;
}

/// @brief Writes the QOI metadata information to the file
/// @param desc QOI descriptor containing the metadata information to write to the file
/// @param dest The destination to write the metadata information to
void write_qoi_header(qoi_desc_t *desc, void* dest)
{
    if (dest == NULL || desc == NULL) return;

    uint8_t *byte = (uint8_t*)dest;

    /* Write the magic characters to the file first */
    *(uint32_t*)byte = qoi_to_be32(*(uint32_t*)QOI_MAGIC);

    /* Writes all the metadata information about the image to the file */

    uint32_t* dimension_ptr = (uint32_t*)&byte[4];

    /* Writes the width and height values of the image to QOI header which stores them in big endian */
    dimension_ptr[0] = qoi_to_be32(desc->width);
    dimension_ptr[1] = qoi_to_be32(desc->height);

    byte[12] = desc->channels;
    byte[13] = desc->colorspace;
}

/// @brief Place the RGB information into the QOI file
/// @param enc QOI encoder
/// @param px The pixel containing the RGB information to place into the QOI opcode
static inline void qoi_enc_rgb(qoi_enc_t *enc, qoi_pixel_t px)
{
    uint8_t tag[4] = {
        QOI_OP_RGB,
        px.red,
        px.green,
        px.blue
    };

    *((uint32_t*)enc->buffer0) = qoi_to_be32(*((uint32_t*)tag));

    enc->buffer_offset += 4;
}

/// @brief Place the RGBA information into the QOI file
/// @param enc QOI encoder
/// @param px The pixel containing the RGBA information to place into the QOI opcode
static inline void qoi_enc_rgba(qoi_enc_t *enc, qoi_pixel_t px)
{
    uint8_t tag[8] =
    {
        QOI_OP_RGBA,
        px.red,
        px.green,
        px.blue,
        px.alpha,
        0,
        0,
        0
    };

    *(uint64_t*)(&enc->buffer0[enc->buffer_offset]) = (*(uint64_t*)tag & 0xFFFFFFFFFF000000) | (enc->buffer0[enc->buffer_offset] & 0x0000000000FFFFFF);

    enc->buffer_offset += 5;
}

/// @brief Place the index position of the buffer into the QOI file
/// @param enc QOI encoder
/// @param index_pos The index position of the buffer to place into the QOI opcode
static inline void qoi_enc_index(qoi_enc_t *enc, uint8_t index_pos)
{
    /* The run-length is stored with a bias of -1 */
    uint8_t tag = QOI_OP_INDEX | index_pos;
    enc->buffer0[enc->buffer_offset++] = tag;
}

/// @brief Place the differences between color values into the QOI opcode
/// @param enc QOI encoder
static inline void qoi_enc_diff(qoi_enc_t *enc, uint8_t red_diff, uint8_t green_diff, uint8_t blue_diff)
{
    uint8_t tag =
        QOI_OP_DIFF |
        (uint8_t)(red_diff + 2) << 4 |
        (uint8_t)(green_diff + 2) << 2 |
        (uint8_t)(blue_diff + 2);

        enc->buffer0[enc->buffer_offset] = tag;
        
        enc->buffer_offset++;
}

/// @brief Place the luma values into the QOI opcode
/// @param enc QOI encoder
static inline void qoi_enc_luma(qoi_enc_t *enc, uint8_t green_diff, uint8_t dr_dg, uint8_t db_dg)
{
    uint8_t tag[2] = {
        QOI_OP_LUMA | (uint8_t)(green_diff + 32), 
        (uint8_t)(dr_dg + 8) << 4 | (uint8_t)(db_dg + 8)
    };

    *(uint16_t*)&enc->buffer0[enc->buffer_offset] = *(uint16_t*)tag;

    enc->buffer_offset += 2;
}

/// @brief Place the run length of a pixel color information into the QOI opcode
/// @param enc QOI encoder
static inline void qoi_enc_run(qoi_enc_t *enc)
{
    /* The run-length is stored with a bias of -1 */
    uint8_t tag = QOI_OP_RUN | (enc->run - 1);
    enc->run = 0;
    
    enc->buffer0[enc->buffer_offset++] = tag;
}

/// @brief Encode pixel data into QOI opcodes
/// @param enc QOI encoder
void qoi_encode_chunk(qoi_desc_t *desc, qoi_enc_t *enc, void *qoi_pixel_bytes)
{

    /* 
        Assume that the pixel byte order is the following below
        bytes[0] = red;
        bytes[1] = green;
        bytes[2] = blue;
        bytes[3] = alpha;
    */

    qoi_pixel_t cur_pixel = *((qoi_pixel_t*)qoi_pixel_bytes);

    /* Assume an RGB pixel with three channels has an alpha value that makes pixels opaque */
    if (desc->channels < 4) 
        cur_pixel.alpha = 255;

    uint8_t index_pos = qoi_get_index_position(cur_pixel);

    /* Increment run length by 1 if pixels are the same */
    if (qoi_cmp_pixel(cur_pixel, enc->prev_pixel, desc->channels))
    {
        /*  Note that the runlengths 63 and 64 (b111110 and b111111) are illegal as they are
            occupied by the QOI_OP_RGB and QOI_OP_RGBA tags. */
        if (++enc->run >= 62 || enc->pixel_offset >= enc->len)
        {
            qoi_enc_run(enc);
        }
    }
    else
    {
        if (enc->run > 0)
        {
            /*  Write opcode for because there are differences in pixels
                The run-length is stored with a bias of -1 */
            qoi_enc_run(enc);
        }
        
        /* Check if pixels exist in one of the pixel hash buffers */
        if (qoi_cmp_pixel(enc->pix_buffer[index_pos], cur_pixel, 4))
        {
            qoi_enc_index(enc, index_pos);
        }
        else
        {
            enc->pix_buffer[index_pos] = cur_pixel;

            /* QOI doesn't have opcodes for alpha values so check alpha values between two pixels first */
            if (desc->channels > 3 && cur_pixel.alpha != enc->prev_pixel.alpha)
            {
                qoi_enc_rgba(enc, cur_pixel);
            }
            else
            {
                /* Check the difference between color values to determine opcode */
                int8_t red_diff, green_diff, blue_diff;
                int8_t dr_dg, db_dg;

                red_diff = cur_pixel.red - enc->prev_pixel.red;
                green_diff = cur_pixel.green - enc->prev_pixel.green;
                blue_diff = cur_pixel.blue - enc->prev_pixel.blue;
                
                dr_dg = red_diff - green_diff;
                db_dg = blue_diff - green_diff;

                if (
                    red_diff >= -2 && red_diff <= 1 &&
                    green_diff >= -2 && green_diff <= 1 &&
                    blue_diff >= -2 && blue_diff <= 1
                )
                {
                    qoi_enc_diff(enc, red_diff, green_diff, blue_diff);
                }

                else if (
                    dr_dg >= -8 && dr_dg <= 7 &&
                    green_diff >= -32 && green_diff <= 31 &&
                    db_dg >= -8 && db_dg <= 7
                )
                {
                    qoi_enc_luma(enc, green_diff, dr_dg, db_dg);
                }

                /* otherwise write an RGB tag containting the RGB values of a pixel */
                else
                {
                    qoi_enc_rgb(enc, cur_pixel);
                }
            }

        }
    }

    /* Advance the pixel offset by one and sets the previous pixel to the current pixel */
    enc->prev_pixel = cur_pixel;
    enc->pixel_offset++;

}

/*  */
/// @brief Initalize the QOI encoder to the default state
/// @param enc QOI encoder
/// @return If the encoder initialized successfully
bool qoi_enc_init(qoi_desc_t* desc, qoi_enc_t* enc)
{
    if (enc == NULL || desc == NULL) return false;

    for (uint8_t element = 0; element < 64; element++) {
        /* Initalize all the pixels in the buffer to zero for each channel of each pixels */
        qoi_initalize_pixel(&enc->pix_buffer[element]); 
    }

    enc->len = (size_t)desc->width * (size_t)desc->height;

    enc->run = 0;
    enc->pixel_offset = 0;
    enc->pixels_written = 0;

    /*  
        The decoder and encoder start with 
        {r: 0, g: 0, b: 0, a: 255}
        as the previous pixel value. 
    */
    qoi_set_pixel_rgba(&enc->prev_pixel, 0, 0, 0, 255);

    return true;
}

/// @brief Checks if the encoder has finished processing the image
/// @param enc QOI encoder
/// @return If the encoder has finished encoding all the pixels of the image
bool qoi_enc_done(qoi_enc_t* enc)
{
    return (enc->pixel_offset >= enc->len); /* Has the encoder encoded all the pixels yet? */
}

#ifdef __cplusplus
}
#endif

#endif // QOI_ENC_N64_H
