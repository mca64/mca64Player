V=1

# [1] Directories
SOURCE_DIR = src
BUILD_DIR = build
ROMFS_DIR = romfs
ROMFS_IMAGE = $(BUILD_DIR)/romfs.dfs

# [2] Source files and assets
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/utils.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/menu.o $(BUILD_DIR)/arena.o $(BUILD_DIR)/cpu_usage.o $(BUILD_DIR)/vu.o $(BUILD_DIR)/hud.o
ASSETS = $(ROMFS_DIR)/sound.wav64

# [3] ROM title
N64_ROM_TITLE = "mca64Player"

# [4] Use libdragon's n64.mk
include $(N64_INST)/include/n64.mk

# [5] Override default DFS root
N64_MKDFS_ROOT = $(ROMFS_DIR)

# [6] Main build target
all: mca64Player.z64
.PHONY: all

# [7] Create DFS image from assets
$(ROMFS_IMAGE): $(ASSETS)
	@mkdir -p $(BUILD_DIR)
	$(N64_MKDFS) $@ $(ROMFS_DIR)

# [8] Compile ELF with assets
$(BUILD_DIR)/mca64Player.elf: $(OBJS) $(ROMFS_IMAGE)

# [9] Create ROM with embedded filesystem
mca64Player.z64: $(BUILD_DIR)/mca64Player.elf $(ROMFS_IMAGE)

# [10] Compile source files
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -o $@ $<

# [11] Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) *.z64 *.v64
.PHONY: clean

# [12] Dependency handling
-include $(wildcard $(BUILD_DIR)/*.d)