/*

    qoi_n64_scr.c

    This file is the entry point of the N64 QOI Encoder ROM

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
#include <stdlib.h>

#include <libdragon.h>
#include "../libdragon/include/joypad.h"
#include "../libdragon/include/joybus.h"

#include "qoi_enc_n64.h"
#include "colorconv.h"


/// @file qoi_n64_scr.c
/// @brief This file contains code for N64 ROM for taking screenshots of whatever displayed on N64 to QOI file.

/// @brief Saves the screenshot to the SD card if it somehow exists.
/// @param disp 
/// @param filename 
/// @return Success of saving the screenshot
bool save_screenshot(surface_t* disp, const char* filename) {
    if (disp == NULL) return false;
    // Get the framebuffer data
    uint16_t* framebuffer = (uint16_t*)disp->buffer;

    // Save the screenshot using the QOI encoder
    qoi_desc_t desc = {
        .width = 320,
        .height = 240,
        .channels = 3, // RGB format
        .colorspace = QOI_SRGB
    };

    qoi_set_dimensions(&desc, 320, 240); // Resolution of the N64 framebuffer

    qoi_set_channels(&desc, 3); // RGB format because the N64 framebuffer does not have an alpha channel
    
    qoi_set_colorspace(&desc, QOI_SRGB); // The N64 framebuffer uses sRGB color space

    qoi_enc_t enc; 
    qoi_enc_init(&desc, &enc); // Initialize the encoder with the descriptor

    FILE* fp = fopen(filename, "wb");
    if (!fp)
    {

        return false;
    }
    
    // Write the QOI header
    uint8_t header[14];
    write_qoi_header(&desc, header);
    fwrite(header, 1, sizeof(header), fp);

    // Encode the pixel data
    for (uint32_t px = 0; px < enc.len; px++)
    {
        uint32_t rgba = n64_color16_to_rgba32(framebuffer[px]);
        qoi_encode_chunk(&desc, &enc, &rgba);
        
        if (enc.pixels_written >= enc.len) {
            fwrite(enc.buffer0, 1, enc.len, fp);
            break;
        };

        // Write the buffer to the file when it is almost full,
        // leaving space for the padding bytes at the end of the file
        if (enc.buffer_offset >= 4096-8) { 
            fwrite(enc.buffer0, 1, enc.buffer_offset, fp);
            enc.buffer_offset = 0;
        }
        
    }

    fwrite(QOI_PADDING, 8, sizeof(QOI_PADDING), fp); // Write the padding bytes
    fclose(fp);
    return true;
}

bool save_screenshot_null(surface_t* disp) {
    if (disp == NULL) return false;
    // Get the framebuffer data
    uint16_t* framebuffer = (uint16_t*)disp->buffer;

    // Save the screenshot using the QOI encoder
    qoi_desc_t desc = {
        .width = 320,
        .height = 240,
        .channels = 3, // RGB format
        .colorspace = QOI_SRGB
    };

    qoi_set_dimensions(&desc, 320, 240); // Resolution of the N64 framebuffer

    qoi_set_channels(&desc, 3); // RGB format because the N64 framebuffer does not have an alpha channel
    
    qoi_set_colorspace(&desc, QOI_SRGB); // The N64 framebuffer uses sRGB color space

    qoi_enc_t enc; 
    qoi_enc_init(&desc, &enc); // Initialize the encoder with the descriptor

    // Write the QOI header
    uint8_t header[14];
    write_qoi_header(&desc, header);

    // Encode the pixel data
    for (uint32_t px = 0; px < enc.len; px++)
    {
        uint32_t rgba = n64_color16_to_rgba32(framebuffer[px]);
        qoi_encode_chunk(&desc, &enc, &rgba);
        
        if (enc.pixels_written >= enc.len) {
            break;
        };

        // Write the buffer to the file when it is almost full,
        // leaving space for the padding bytes at the end of the file
        if (enc.buffer_offset >= 4096-8) { 
            enc.buffer_offset = 0;
        }
        
    }

    return true;
}

/// @brief Saves the raw screenshot to the SD card if it somehow exists.
bool save_screenshot_raw(surface_t* disp, const char* filename) {

    // Get the framebuffer data
    uint16_t* framebuffer = (uint16_t*)disp->buffer;

    FILE* fp = fopen(filename, "wb");
    if (!fp)
    {
        return false;
    }
    
    // Write the raw pixel data to the file
    fwrite(framebuffer, sizeof(uint16_t), 320 * 240, fp);
    
    fclose(fp);
    return true;
}

/// @brief Entry point for this N64 ROM.
/// @param  nothing because we are running this on the N64 and we don't have access to command line arguments
/// @return nothing
int main(void) {

    float x = 0.0f;
    float y = 0.0f;

    float speed = 0.1f;

    bool toRight = true;
    bool toDown = true;

    float encodedTime = 0.0f;

    rdpq_font_t *font;

    // Initialize libdragon subsystems
    debug_init_isviewer();
    console_init();

    debug_init_usblog();
    console_set_debug(true);

    timer_init();
    joypad_init();
    rdpq_init();

    dfs_init(DFS_DEFAULT_LOCATION);

    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);

    font = rdpq_font_load_builtin(FONT_BUILTIN_DEBUG_MONO);
    rdpq_text_register_font(1, font);

    sprite_t* logo = sprite_load("rom://n64brew.sprite");
    uint64_t start = timer_ticks(), end = timer_ticks();
    float delta = 0.0f;
    while (1) {
        // Start drawing to the screen
        surface_t* disp;

        joypad_port_t port = JOYPAD_PORT_1;

        joypad_poll();
        joypad_buttons_t pressed = joypad_get_buttons_pressed(port);

        end = timer_ticks();
        delta = TIMER_MICROS(end - start) / 1000.0f; // Convert to milseconds
        start = end;

        while(!(disp = display_try_get())) {;}

        // Move the logo around the screen like a DVD logo to
        // show that the screen is being updated and to have something to screenshot. 
        // The logo will bounce around the screen and change direction when it hits the edge of the screen.
        if (toDown) {
            if(y < 240.0f - (logo->height * 0.5f)) {
                y += speed * delta;
            }
            else {
                toDown = false;
            }
        }
        else {
            if (y > 0.0f) {
                y -= speed * delta;
            }
            else {
                toDown = true;
            }    
        }

        if (toRight) {
            if (x < 320.0f - (logo->width * 0.5)) {
                x += speed * delta;
            }
            else {
                toRight = false;
            }
        }
        else {
            if (x > 0.0f) {
                x -= speed * delta;
            }
            else {
                toRight = true;
            }
        }

        rdpq_attach(disp, NULL);

        rdpq_set_mode_fill(RGBA32(0, 0, 255, 255));
        rdpq_fill_rectangle(0, 0, 320, 240);

        rdpq_set_mode_copy(true);
        rdpq_sprite_blit(logo, (int)x, (int)y, &( rdpq_blitparms_t ) {
            .scale_x = 0.5f,
            .scale_y = 0.5f
        });

        rdpq_set_mode_standard();

        rdpq_text_printf(&(rdpq_textparms_t) {
                .width = 320-32,
                .align = ALIGN_LEFT,
                .wrap = WRAP_WORD,
            }, 1, 32, 32, "Encoded in %.2f ms", encodedTime);

        
        if (pressed.b) {
            FILE* fp = fopen("sd://screenshot.raw", "wb");
            if (fp) {
                fclose(fp);
                save_screenshot_raw(disp, "sd://screenshot.raw");
            }
        }
        else if (pressed.a) {
            FILE* fp = fopen("sd://screenshot.qoi", "wb");

            if (fp) {
                
                fclose(fp);
                float startEncode = timer_ticks();
                save_screenshot(disp, "sd://screenshot.qoi");
                float endEncode = timer_ticks();
                encodedTime = TIMER_MICROS(endEncode - startEncode) / 1000.0f; // Convert to milliseconds
            }
            else {
                float startEncode = timer_ticks();
                save_screenshot_null(disp);
                float endEncode = timer_ticks();
                encodedTime = TIMER_MICROS(endEncode - startEncode) / 1000.0f; // Convert to milliseconds
            }
        }

        rdpq_detach_show();

    }

    return 0;
}