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

/// @brief Checks what type of screenshot was taken to perform different actions based on the type of screenshot taken, such as displaying different information on the screen or saving the screenshot in a different way.
enum whichScreenshotType {
    /// @brief No screenshot has been taken yet
    SCREENSHOT_NEVER_TAKEN,
    /// @brief The screenshot was saved to null, which is used for testing the performance of the encoder without the bottleneck of writing to the SD card
    SCREENSHOT_TYPE_NULL,
    /// @brief The screenshot was saved as a raw file, which is used for testing the performance of writing to the SD card without the bottleneck of encoding the image
    SCREENSHOT_TYPE_RAW,
    /// @brief The screenshot was saved as a QOI file, which is the main functionality of this project
    SCREENSHOT_TYPE_QOI,
    /// @brief The screenshot was captured into a buffer but not saved, which is used for testing the performance of capturing the framebuffer without the bottleneck of encoding or writing to the SD card
    SCREENSHOT_FRAME_CAPTURED
} whichScreenshotType;

/// @file qoi_n64_scr.c
/// @brief This file contains code for N64 ROM for taking screenshots of whatever displayed on N64 to QOI file.

/// @brief Saves the screenshot to the SD card if it somehow exists.
/// @param disp 
/// @param filename 
/// @return Success of saving the screenshot
bool save_screenshot(surface_t* disp, const char* filename, uint32_t *bytesWritten) {
    if (disp == NULL || bytesWritten == NULL) return false;
    // Get the framebuffer data
    uint16_t* framebuffer = (uint16_t*)disp->buffer;

    // Save the screenshot using the QOI encoder
    qoi_desc_t desc;

    qoi_set_dimensions(&desc, 320, 240); // Resolution of the N64 framebuffer

    qoi_set_channels(&desc, 3); // RGB format because the N64 framebuffer does not have an alpha channel
    
    qoi_set_colorspace(&desc, QOI_SRGB); // The N64 framebuffer uses sRGB color space

    qoi_enc_t enc; 
    qoi_enc_init(&desc, &enc); // Initialize the encoder with the descriptor

    FILE* fp = fopen(filename, "wb");
    *bytesWritten = 0;
    if (!fp)
    {
        return false;
    }
    
    // Write the QOI header
    uint8_t header[14];
    write_qoi_header(&desc, header);
    fwrite(header, 1, sizeof(header), fp);

    *bytesWritten += 14;

    // Encode the pixel data
    for (uint32_t px = 0; px < enc.len; px++)
    {
        uint32_t rgba = n64_color16_to_rgba32(framebuffer[px]);
        qoi_encode_chunk(&desc, &enc, &rgba);
        enc.pixels_written++;

        if (enc.pixels_written >= enc.len) {
            fwrite(enc.buffer0, 1, enc.buffer_offset, fp);
            *bytesWritten += enc.buffer_offset;
            break;
        };

        // Write the buffer to the file when it is almost full,
        // leaving space for the padding bytes at the end of the file
        if (enc.buffer_offset >= 4096-8) { 
            fwrite(enc.buffer0, 1, enc.buffer_offset, fp);
            *bytesWritten += enc.buffer_offset;
            enc.buffer_offset = 0;
        }
        
    }

    fwrite(QOI_PADDING, sizeof(uint8_t), 8, fp); // Write the padding bytes

    *bytesWritten += 8;

    fclose(fp);
    return true;
}

/// @brief Saves the screenshot to null, which is used for testing the performance of the encoder without the bottleneck of writing to the SD card.
/// @param disp
/// @return Success of saving the screenshot
bool save_screenshot_null(surface_t* disp, uint32_t *bytesWritten) {
    if (disp == NULL || bytesWritten == NULL) return false;
    // Get the framebuffer data
    uint16_t* framebuffer = (uint16_t*)disp->buffer;

    // Save the screenshot using the QOI encoder
    qoi_desc_t desc;
    *bytesWritten = 0;

    qoi_set_dimensions(&desc, 320, 240); // Resolution of the N64 framebuffer

    qoi_set_channels(&desc, 3); // RGB format because the N64 framebuffer does not have an alpha channel
    
    qoi_set_colorspace(&desc, QOI_SRGB); // The N64 framebuffer uses sRGB color space

    qoi_enc_t enc; 
    qoi_enc_init(&desc, &enc); // Initialize the encoder with the descriptor

    // Write the QOI header
    uint8_t header[14];
    write_qoi_header(&desc, header);

    *bytesWritten += 14; // Account for the header bytes

    // Encode the pixel data
    for (uint32_t px = 0; px < enc.len; px++)
    {
        uint32_t rgba = n64_color16_to_rgba32(framebuffer[px]);
        qoi_encode_chunk(&desc, &enc, &rgba);

        enc.pixels_written++;

        if (enc.pixels_written >= enc.len) {
            *bytesWritten += enc.buffer_offset;
            break;
            
        };

        // Write the buffer to the file when it is almost full,
        // leaving space for the padding bytes at the end of the file
        if (enc.buffer_offset >= 4096-8) { 
            *bytesWritten += enc.buffer_offset;
            enc.buffer_offset = 0;
        }
        
    }

    bytesWritten += 8; // Account for the padding bytes

    return true;
}

/// @brief Saves the raw screenshot to the SD card if it somehow exists.
bool save_screenshot_raw(surface_t* disp, const char* filename, uint32_t *bytesWritten) {
    if (disp == NULL || bytesWritten == NULL || filename == NULL) return false;
    // Get the framebuffer data
    uint16_t* framebuffer = (uint16_t*)disp->buffer;
    *bytesWritten = 0;

    FILE* fp = fopen(filename, "wb");
    if (!fp)
    {
        return false;
    }
    
    // Write the raw pixel data to the file
    fwrite(framebuffer, sizeof(uint16_t), 320 * 240, fp);
    *bytesWritten = 320 * 240 * sizeof(uint16_t);
    
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

    enum whichScreenshotType scrType = SCREENSHOT_NEVER_TAKEN;
    bool successfulSave = false;
    surface_t lastSavedFrame = surface_alloc(FMT_RGBA16, 320, 240);

    uint32_t bytesWritten = 0;

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

    sprite_t* logo = sprite_load("rom:/n64brew.sprite");
    sprite_t* background = sprite_load("rom:/background.sprite");
    uint64_t start = timer_ticks(), end = timer_ticks();
    float delta = 0.0f;

    debug_init_sdfs("sd:/", -1);
    while (1) {
        // Start drawing to the screen
        surface_t* disp;

        joypad_port_t port = JOYPAD_PORT_1;

        joypad_poll();
        joypad_buttons_t pressed = joypad_get_buttons_pressed(port);
        joypad_buttons_t held = joypad_get_buttons_held(port);

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

        rdpq_set_mode_copy(true);
        rdpq_sprite_blit(background, 0, 0, NULL);

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
            }, 1, 32, 32, "Encoded in %.8f ms, bytes written: %lu bytes", encodedTime, bytesWritten);

        rdpq_text_printf(&(rdpq_textparms_t) {
            .width = 320-32,
            .align = ALIGN_LEFT,
            .wrap = WRAP_WORD,
        }, 1, 32, 60, "State: %s", scrType == SCREENSHOT_NEVER_TAKEN ? "No screenshot taken" : 
            scrType == SCREENSHOT_TYPE_NULL ? "Saved to null" :
            scrType == SCREENSHOT_TYPE_RAW ? "Saved as raw" :
            scrType == SCREENSHOT_TYPE_QOI ? "Saved as QOI" :
            scrType == SCREENSHOT_FRAME_CAPTURED ? "Frame captured, not saved" : "Unknown");

        rdpq_text_printf(&(rdpq_textparms_t) {
            .width = 320-32,
            .align = ALIGN_LEFT,
            .wrap = WRAP_WORD,
        }, 1, 32, 72, "Save successful: %s", successfulSave ? "Yes" : "No");
        
        if (pressed.z) {

            // capture the current frame into a buffer for testing
            rdpq_attach(&lastSavedFrame, NULL);
            rdpq_set_mode_copy(false);
            rdpq_tex_blit(disp, 0, 0, NULL);

            scrType = SCREENSHOT_FRAME_CAPTURED;
            successfulSave = true;

            rdpq_detach();
        }
        
        if (held.d_left) {
            rdpq_set_mode_copy(true);
            rdpq_tex_blit(&lastSavedFrame, 0, 0, NULL);
        }

        if (pressed.b) {

            surface_t img = surface_alloc(FMT_RGBA16, 320, 240);

            rdpq_attach(&img, NULL);
            rdpq_set_mode_copy(false);
            rdpq_tex_blit(disp, 0, 0, NULL);

            float startEncode = timer_ticks();
            successfulSave = save_screenshot_raw(&img, "sd:/screenshot.raw", &bytesWritten);
            float endEncode = timer_ticks();
            encodedTime = TIMER_MICROS(endEncode - startEncode) / 1000.0f; // Convert to milliseconds

            scrType = SCREENSHOT_TYPE_RAW;

            rdpq_detach();

            surface_free(&img);
            
        }
        else if (pressed.a) {
            surface_t img = surface_alloc(FMT_RGBA16, 320, 240);
            
            rdpq_attach(&img, NULL);
            rdpq_set_mode_copy(false);
            rdpq_tex_blit(disp, 0, 0, NULL);
            rdpq_detach();

                
            float startEncode = timer_ticks();
            successfulSave = save_screenshot(&img, "sd:/screenshot.qoi", &bytesWritten);
            float endEncode = timer_ticks();
            encodedTime = TIMER_MICROS(endEncode - startEncode) / 1000.0f; // Convert to milliseconds
            scrType = SCREENSHOT_TYPE_QOI;
            
            surface_free(&img);
        }
        else if (pressed.d_up) {
            surface_t img = surface_alloc(FMT_RGBA16, 320, 240);
            
            rdpq_attach(&img, NULL);
            rdpq_set_mode_copy(false);
            rdpq_tex_blit(disp, 0, 0, NULL);
            rdpq_detach();

            float startEncode = timer_ticks();
            successfulSave = save_screenshot_null(&img, &bytesWritten);
            float endEncode = timer_ticks();
            encodedTime = TIMER_MICROS(endEncode - startEncode) / 1000.0f; // Convert to milliseconds
            scrType = SCREENSHOT_TYPE_NULL;

            surface_free(&img);

        }

        rdpq_detach_show();

    }

    return 0;
}