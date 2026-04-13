# Supplementary Tools for N64 QOI Encoder Demo

![A text that says QOI with a beer bottle behind QOI text with N64 logo on top](../docs_assets/n64_qoi_logo_256px.png)

A small, QOI (Quite OK Image) encoder example program for converting raw screenshots taken from a real N64 to QOI files.

This example program only supports RGBA5551 format that is commonly used in N64 framebuffer.

## Requirements
- A N64 flashcart with SD card support
  - ### Example Flash Carts with SD card support
    - [SummerCart64](https://summercart64.dev/)
    - [EverDrive-64 X7](https://krikzz.com/our-products/cartridges/ed64x7.html)

- C compiler
- Make

## How to Build
This tutorial assumes you have make and a C compiler installed.

Run this command to build the program

```bash
make
```

To clear build files, run this command

```bash
make clean
```

## How to Run

use this command to run this program

```bash
./qoi_enc screenshot.raw 320 240 screenshot.qoi
```

---

## Tools

- [Framebuffer Converter](https://aftersol.github.io/n64_raw_framebuffer_converter/) Converts raw screenshot captured from N64 to PNG