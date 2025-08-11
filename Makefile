V=1

# Katalogi
SOURCE_DIR = src
BUILD_DIR = build
ROMFS_DIR = romfs
ROMFS_IMAGE = $(BUILD_DIR)/romfs.dfs

# Pliki źródłowe i zasoby
OBJS = $(BUILD_DIR)/main.o
ASSETS = $(ROMFS_DIR)/sound.wav64

# Tytuł ROM-u
N64_ROM_TITLE = "Hello WAV64"

# Użyj libdragonowego n64.mk
include $(N64_INST)/include/n64.mk

# Nadpisz domyślny katalog DFS
N64_MKDFS_ROOT = $(ROMFS_DIR)

# Główne cele
all: hello.z64
.PHONY: all

# Tworzenie pliku DFS z zasobami
$(ROMFS_IMAGE): $(ASSETS)
	@mkdir -p $(BUILD_DIR)
	$(N64_MKDFS) $@ $(ROMFS_DIR)

# Kompilacja ELF z zasobami
$(BUILD_DIR)/hello.elf: $(OBJS) $(ROMFS_IMAGE)

# Tworzenie ROM-u z dołączonym systemem plików
hello.z64: $(BUILD_DIR)/hello.elf $(ROMFS_IMAGE)

# Kompilacja źródeł
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -o $@ $<

# Czyszczenie
clean:
	rm -rf $(BUILD_DIR) *.z64 *.v64
.PHONY: clean

# Obsługa zależności
-include $(wildcard $(BUILD_DIR)/*.d)