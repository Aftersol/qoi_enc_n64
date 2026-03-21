# QOI Encoder Demo for N64

https://github.com/user-attachments/assets/0d024e08-101c-45ba-83ee-db06d3b5e936

Small, QOI (Quite OK Image) encoder demo for Nintendo 64

## Controls

A - save to QOI
B - save to raw
Z - save frame to RAM
DPad-Up - save to /dev/zero (null)
DPad-Left View last frame

## Requirements

- Libdragon Preview branch
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

---

## License

MIT License — see [LICENSE file](./LICENSE) for details.

[purple white and orange light by mymind](https://unsplash.com/photos/purple-white-and-orange-light-tZCrFpSNiIQ) - Licensed under Unsplash License
