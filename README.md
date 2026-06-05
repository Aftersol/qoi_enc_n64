# QOI Encoder Demo for N64

![A text that says QOI with a beer bottle behind QOI text with N64 logo on top](docs_assets/n64_qoi_logo_256px.png)

https://github.com/user-attachments/assets/0d024e08-101c-45ba-83ee-db06d3b5e936

Small, QOI (Quite OK Image) encoder demo for Nintendo 64

## Controls

- A - save to QOI
- B - save to raw
- Z - save frame to RAM
- DPad-Up - save to /dev/zero (null)
- DPad-Left - View last frame
- DPad-Down - Hide Logo
- Start - Hide Logo

## Requirements

- A N64 flashcart with SD card support
  - ### Example Flash Carts with SD card support
    - [SummerCart64](https://summercart64.dev/)
    - [EverDrive-64 X7](https://krikzz.com/our-products/cartridges/ed64x7.html)
    
- [Libdragon Preview branch](https://github.com/DragonMinded/libdragon/tree/preview)
- MIPS64 C compiler
- Make

## How to Build
This tutorial assumes you have your N64 Toolchain set up including GCC for MIPS.
Make sure you are on the preview branch of libdragon.

Clone this repository with `--recurse-submodules` or if you haven't run:

```bash

git submodule update --init
```
---
Initialize libdragon:
```bash
libdragon init
```
Then run make to build this project:

```bash
libdragon make
```

## How to Run
Download the ROM in the [release page](https://github.com/Aftersol/n64_qoi_enc/releases) and put the downloaded qoi_enc.z64 file to the SD card. Then insert the SD card with the qoi_enc.z64 into the N64 flashcart. Insert the N64 flashcart with that SD card installed into the N64 and select the qoi_enc.z64 ROM from the list of ROMs to run. Alternatively, you can build the ROM and insert that ROM into the SD card plug that into the N64 flashcart.

---

## Tools

- [Framebuffer Converter](https://aftersol.github.io/n64_raw_framebuffer_converter/) Converts raw screenshot captured from N64 to PNG

## License

MIT License — see [LICENSE file](https://github.com/Aftersol/n64_qoi_enc/blob/main/LICENSE) for details.
