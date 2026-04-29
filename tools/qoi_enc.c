/*

    -- qoi_enc.c -- N64 framebuffer to QOI program

    - version 1.2.n64.0 -- revised 2026-04-13

    -- Changelog --

    - version 1.1.n64.1 (2026-04-13)
        - Modified program to display version info

    - version 1.1.n64.0 (2026-04-13)
        - Modified program to use modified sQOI library
        - Supports RGBA5551 conversion to RGBA32 for conversion to QOI
        - Updated MIT License copyright year

    - version 1.1 (2025-04-07)
        - Implemented error handling in case reading RGBA file fails
        - Strictly check requested color channels and colorspace
        according to QOI specifications

    - version 1.0 (2025-01-13)
        - Initial release

    MIT License

    Copyright (c) 2024-2026 Aftersol

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qoi_enc_n64.h"
typedef enum {
    ENC_NAME,
    ENC_INPUT_FILENAME,
    ENC_WIDTH,
    ENC_HEIGHT,
    ENC_BITDEPTH,
    ENC_OUTPUT_FILENAME,
} QOI_ENC_ARGS;
#define ENC_BUFFER_SIZE 16384

uint16_t read_be_u16(uint16_t val)
{
    uint8_t* val_ptr = (uint8_t*)&val;
    return (uint16_t)((val_ptr[1] << 0) | (val_ptr[0] << 8));
}

uint32_t read_be_u32(uint32_t val)
{
    uint8_t* val_ptr = (uint8_t*)&val;
    return (uint32_t)((val_ptr[3] << 0) | (val_ptr[2] << 8) | (val_ptr[1] << 16) | (val_ptr[0] << 24));
}

const char version_number[] = "version 1.2.n64.0";
const char revised_date[] = "2026-04-16";

void print_version(void)
{
    printf("QOI Encoder\nversion: %s -- revised %s\n", version_number, revised_date);
}

void print_help(void)
{
    printf("Example usage: qoi_enc <filename> <width> <height> <bitdepth> <output>\n");
    printf("16: 16-bit RGBA5551\n32: 32-bit RGBA32\n");
}

int main(int argc, char* argv[])
{

    qoi_desc_t desc;
    qoi_enc_t enc;
    
    /* Raw file to be opened */
    FILE* fp;
    /* File size of the raw file */
    size_t file_size;

    uint8_t* file_buffer0 = NULL;
    uint8_t* file_buffer1 = NULL;

    uint8_t header[16];
    
    uint32_t width, height;
    uint8_t channels, colorspace;

    uint8_t bitdepth;
    
    /* 16-bit RGBA5551 */
    uint16_t* rgba5551_frame_buffer;

    /* Scratchpad when converted to RGBA32 */
    uint32_t* rgba32_scratch_frame_buffer;

    /* For attaching it to encoder to limit memory consumption */
    uint8_t enc_buffer[ENC_BUFFER_SIZE];

    print_version();
    
    if (argc < 6)
    {
        print_help();
        return -1;
    }
    else
    {

        if (strlen(argv[ENC_INPUT_FILENAME]) <= 0)
        {
            print_help();
            return -1;
        }

        width = strtoul(argv[ENC_WIDTH], NULL, 0);
        height = strtoul(argv[ENC_HEIGHT], NULL, 0);
        bitdepth = strtoul(argv[ENC_BITDEPTH], NULL, 0);

        /* Check for valid bitdepth */
        if (!(bitdepth == 16 || bitdepth == 32)) {
            print_help();
            return -1;
        }

        /* Framebuffer on N64 is always rgba screenshot */
        channels = 4;

        /* The N64 framebuffer uses sRGB color space */
        colorspace = 0;

        if (width < 0 || height < 0)
        {
            print_help();
            return -1;
        }
    }

    printf("Opening %s\n", argv[ENC_INPUT_FILENAME]);

    fp = fopen(argv[ENC_INPUT_FILENAME], "rb");

    if (!fp)
    {
        print_help();
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    
    file_size = ftell(fp);

    if ((size_t)width * (size_t)height * (bitdepth == 16 ? 2 : 4) < file_size)
    {
        size_t image_size = (size_t)width * (size_t)height * (bitdepth == 16 ? 2 : 4);
        size_t size_difference = image_size - file_size;

        printf(
            "%zu %s are required for the file, %s. Your requested image dimensions and bit-depth allocates %zu %s. That is %zu %s difference\n",
            file_size,
            (file_size > 1) ? "bytes" : "byte",
            argv[ENC_INPUT_FILENAME],
            image_size,
            (image_size > 1) ? "bytes" : "byte",
            size_difference, 
            /* for printing plurals from of the word "byte" */
            (size_difference > 1) ? "bytes" : "byte"
            );
        print_help();

        return -1;
    }

    fseek(fp, 0, SEEK_SET);

    file_buffer0 = (uint8_t*)calloc(file_size + 1, sizeof(uint8_t)); /* RGBA5551 framebuffer from file */
    
    if (!file_buffer0)
    {
        fclose(fp);
        return 1;
    }

    if (bitdepth == 16) {
        file_buffer1 = (uint8_t*)calloc(file_size + 1, sizeof(uint8_t)*2); /* RGBA framebuffer */
        if (!file_buffer1)
        {
            fclose(fp);
            free(file_buffer0);
            file_buffer0 = NULL;
            return 1;
        }
    }
    
    if (fread(file_buffer0, 1, file_size, fp) < file_size) 
    {
        if (ferror(fp)) 
        {
            printf("An error has occur while reading %s\n", argv[ENC_INPUT_FILENAME]);
            print_help();
    
            fclose(fp);
            free(file_buffer0);
    
            return 1;
        }
    }

    fclose(fp);

    if (bitdepth == 32) {
        /* RGBA is simple to read */
        rgba32_scratch_frame_buffer = (uint32_t*)file_buffer0;
    } else {
        /* Very convoluted conversion from big endian to RGBA32 */
        rgba5551_frame_buffer = (uint16_t*)file_buffer0;

        for (size_t i = 0; i < file_size / sizeof(uint16_t); i++) {
            
            /* 16-bit rgba5551 to rgba32 logic */
            int r = (read_be_u16(rgba5551_frame_buffer[i]) >> 11) & 0x1F;
            int g = (read_be_u16(rgba5551_frame_buffer[i]) >>  6) & 0x1F;
            int b = (read_be_u16(rgba5551_frame_buffer[i]) >>  1) & 0x1F;
            int a = (read_be_u16(rgba5551_frame_buffer[i]) >>  0) & 0x01;
        
            /* expand to 8 bit */
            r = (r << 3) | (r >> 2);
            g = (g << 3) | (g >> 2);
            b = (b << 3) | (b >> 2);
            a = a * 255;

            /* Store the converted RGBA32 values in the file buffer */
            file_buffer1[i*4] = r;
            file_buffer1[i*4+1] = g;
            file_buffer1[i*4+2] = b;
            file_buffer1[i*4+3] = a;

        }
    
        free(file_buffer0); /* Destroy RGBA5551 file */
        file_buffer0 = NULL;
        rgba5551_frame_buffer = NULL;

        rgba32_scratch_frame_buffer = (uint32_t*)file_buffer1;
    }

    qoi_desc_init(&desc);
    
    qoi_set_dimensions(&desc, width, height);
    qoi_set_channels(&desc, channels);
    qoi_set_colorspace(&desc, colorspace);
    
    fp = fopen(argv[ENC_OUTPUT_FILENAME], "wb");
    if (!fp) {
        goto cleanup;
    }
    
    printf(
        "Encoding %s to %s. Please wait . . .\n",
        argv[ENC_INPUT_FILENAME],
        argv[ENC_OUTPUT_FILENAME]
    );

    qoi_enc_init(&desc, &enc);

    /* qoi_enc_alloc_buffer(&enc, ENC_BUFFER_SIZE); */

    qoi_enc_set_buffer(&enc, enc_buffer, ENC_BUFFER_SIZE, false);

    write_qoi_header(&desc, header);

    fwrite(header, 1, 14, fp);

    /* Begin encoding RGBA32 data to QOI file */
    
    /* Encode the pixel data */
    for (uint32_t px = 0; px < enc.len; px++)
    {
        uint32_t rgba = rgba32_scratch_frame_buffer[px];
        qoi_encode_chunk(&desc, &enc, &rgba);

        if (enc.pixels_written >= enc.len) {
            fwrite(enc.enc_buffer, 1, enc.buffer_offset, fp);
            break;
        };

        /* Write the buffer to the file when it is almost full,
           leaving space for the padding bytes at the end of the file */
        if (enc.buffer_offset >= enc.buffer_len-8) { 
            fwrite(enc.enc_buffer, 1, enc.buffer_offset, fp);
            qoi_enc_reset_buffer(&enc);
        }
        
    }

    /* Write QOI padding per QOI specifications */
    fwrite(QOI_PADDING, sizeof(uint64_t), 1, fp);
    
    /* qoi_enc_free_buffer(&enc); */

    /* Finish writing file */
    fclose(fp);

    printf("Done\n");

    cleanup:

    if (bitdepth == 32) {
        free(file_buffer0);
        file_buffer0 = NULL;
    } else {
        /* Destroy RGBA32 file */
        free(file_buffer1);
        file_buffer1 = NULL;
        rgba32_scratch_frame_buffer = NULL;
    }

    rgba32_scratch_frame_buffer = NULL;

    return 0;
}
