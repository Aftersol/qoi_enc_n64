/*

    -- example_enc.c -- Reference QOI encoding usage of this library

    -- version 1.1 -- revised 2025-04-07

    -- Changelog --

    - version 1.1 (2025-04-07)
        - Implemented error handling in case reading RGBA file fails
        - Strictly check requested color channels and colorspace
        according to QOI specifications

    - version 1.0 (2025-01-13)

    MIT License

    Copyright (c) 2024-2025 Aftersol

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

void print_help()
{
    printf("Example usage: example_enc <filename> <width> <height> <output>\n");
}

int main(int argc, char* argv[])
{

    qoi_desc_t desc;
    qoi_enc_t enc;
    uint8_t* header, *file_buffer0, *file_buffer1;
    FILE* fp;
    uint32_t width, height;
    uint8_t channels, colorspace;
    size_t file_size;
    uint16_t* frame_buffer;
    uint32_t* frame_buffer32;

    const uint32_t enc_buffer_size = 512;
    
    if (argc < 4)
    {
        print_help();
        return -1;
    }
    else
    {

        if (strlen(argv[1]) <= 0)
        {
            print_help();
            return -1;
        }

        width = strtoul(argv[2], NULL, 0);
        height = strtoul(argv[3], NULL, 0);

        /* framebuffer on n64 is always rgba screenshot */
        channels = 4;

        colorspace = 0;

        if (width < 0 || height < 0)
        {
            print_help();
            return -1;
        }
    }

    printf("Opening %s\n",argv[1]);

    fp = fopen(argv[1], "rb");

    if (!fp)
    {
        print_help();
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    
    file_size = ftell(fp);

    /*if ((size_t)width * (size_t)height * (size_t)channels < file_size)
    {
        size_t image_size = (size_t)width * (size_t)height * (size_t)channels;
        size_t size_difference = file_size - image_size;

        printf(
            "%zu bytes are required for the file, %s. Your requested dimensions and channels amount allocates %zu bytes. That is %zu %s difference\n",
            file_size,
            argv[1],
            image_size,
            size_difference,
            /* for printing plurals from of the word "byte" */
            /*(size_difference > 1) ? "bytes" : "byte"
            );
        print_help();

        return -1;
    }*/

    fseek(fp, 0, SEEK_SET);

    file_buffer0 = (uint8_t*)calloc(file_size + 1, sizeof(uint8_t));
    file_buffer1 = (uint8_t*)calloc(file_size + 1, sizeof(uint8_t)*2);
    
    if (!file_buffer0 || !file_buffer1)
    {
        fclose(fp);
        return 1;
    }

    if (fread(file_buffer0, 1, file_size, fp) < file_size) 
    {
        if (ferror(fp)) 
        {
            printf("An error has occur while reading %s\n", argv[1]);
            print_help();
    
            fclose(fp);
            free(file_buffer0);
    
            return 1;
        }
    }

    fclose(fp);

    frame_buffer = (uint16_t*)file_buffer0;

    for (size_t i = 0; i < file_size / sizeof(uint16_t); i++) {
            
        int r = (read_be_u16(frame_buffer[i]) >> 11) & 0x1F;
        int g = (read_be_u16(frame_buffer[i]) >>  6) & 0x1F;
        int b = (read_be_u16(frame_buffer[i]) >>  1) & 0x1F;
        int a = (read_be_u16(frame_buffer[i]) >>  0) & 0x01;
    
        // expand to 8 bit
        r = (r << 3) | (r >> 2);
        g = (g << 3) | (g >> 2);
        b = (b << 3) | (b >> 2);
        a = a * 255;

        if (channels == 4)
        {
            file_buffer1[i*4] = r;
            file_buffer1[i*4+1] = g;
            file_buffer1[i*4+2] = b;
            file_buffer1[i*4+3] = a;
        }
        else {
            file_buffer1[i*3] = r;
            file_buffer1[i*3+1] = g;
            file_buffer1[i*3+2] = b;
        }

    }

    header = (uint8_t*)calloc(32, sizeof(uint8_t));

    qoi_desc_init(&desc);
    
    qoi_set_dimensions(&desc, width, height);
    qoi_set_channels(&desc, channels);
    qoi_set_colorspace(&desc, colorspace);
    
    fp = fopen(argv[4], "wb");
    if (!fp) {
        free(file_buffer0);
        free(file_buffer1);
        free(header);
        return 1;
    }
    
    printf("Encoding %s to %s. Please wait . . .\n", argv[1], argv[4]);

    qoi_enc_init(&desc, &enc);
    qoi_enc_alloc_buffer(&enc, enc_buffer_size);
    qoi_enc_reset_buffer(&enc);

    write_qoi_header(&desc, header);

    fwrite(header, 1, 14, fp);

    free(header);
    header = NULL;
    
    frame_buffer32 = (uint32_t*)file_buffer1;
    /* Encode the pixel data */
    for (uint32_t px = 0; px < enc.len; px++)
    {
        uint32_t rgba = frame_buffer32[px];
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
    fwrite(QOI_PADDING, sizeof(uint64_t), 1, fp);

    fclose(fp);

    printf("Done\n");


    free(file_buffer0);
    free(file_buffer1);

    return 0;
}