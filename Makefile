#
#   Copyright (c) 2026 Aftersol
#
#   Permission is hereby granted, free of charge, to any person obtaining a copy
#   of this software and associated documentation files (the "Software"), to deal
#   in the Software without restriction, including without limitation the rights
#   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#   copies of the Software, and to permit persons to whom the Software is
#   furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included in all
#   copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#   SOFTWARE.
#

SOURCE_DIR=src
BUILD_DIR=build
include $(N64_INST)/include/n64.mk

all: qoi_enc.z64
.PHONY: all

FILESYSTEM_DIR = filesystem

assets = $(FILESYSTEM_DIR)/n64brew.png
assets_conv = $(addprefix filesystem/,$(notdir $(assets:%.png=%.sprite)))

AUDIOCONV_FLAGS ?=
MKSPRITE_FLAGS ?=

OBJS = $(BUILD_DIR)/main.o

$(FILESYSTEM_DIR)/%.sprite: $(FILESYSTEM_DIR)/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o $(FILESYSTEM_DIR) "$<"

qoi_enc.z64: N64_ROM_TITLE="qoiScreenshot"
qoi_enc.z64: $(BUILD_DIR)/qoi_enc.dfs

$(BUILD_DIR)/qoi_enc.elf: $(OBJS)
$(BUILD_DIR)/qoi_enc.dfs: $(assets_conv)
	@echo "	[DFS] $@"
	if [ ! -s "$<"]; then rm -f "$<"; fi
	$(N64_MKDFS) "$@" $(FILESYSTEM_DIR) >/dev/null

clean:
	rm -f $(BUILD_DIR)/* *.z64
.PHONY: clean

-include $(wildcard $(BUILD_DIR)/*.d)