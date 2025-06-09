# Liberation Circuit - Modern Cross-Platform Makefile
# Supports GCC, Clang/LLVM, and MSVC on Linux, macOS, and Windows

# Platform detection
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)
UNAME_M := $(shell uname -m 2>/dev/null || echo x86_64)

# Platform-specific settings
ifeq ($(UNAME_S),Windows)
    PLATFORM := windows
    EXE_EXT := .exe
    NETWORK_LIBS := -lws2_32
    OBJ_EXT := .obj
else ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
    PLATFORM := windows
    EXE_EXT := .exe
    NETWORK_LIBS := -lws2_32
    OBJ_EXT := .o
else ifeq ($(findstring MSYS,$(UNAME_S)),MSYS)
    PLATFORM := windows
    EXE_EXT := .exe
    NETWORK_LIBS := -lws2_32
    OBJ_EXT := .o
else ifeq ($(UNAME_S),Darwin)
    PLATFORM := macos
    EXE_EXT :=
    NETWORK_LIBS :=
    OBJ_EXT := .o
else
    PLATFORM := linux
    EXE_EXT :=
    NETWORK_LIBS :=
    OBJ_EXT := .o
endif

# Compiler detection (prioritize Clang/LLVM, make GCC optional)
CC := $(or $(CC),$(shell which clang 2>/dev/null || which clang-cl 2>/dev/null || which gcc 2>/dev/null || echo clang))
COMPILER_TYPE := $(shell $(CC) --version 2>/dev/null | head -1)

# Compiler-specific warning flags
ifeq ($(findstring clang,$(COMPILER_TYPE)),clang)
    WARNINGS := -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unused-variable
    COMPILER_NAME := Clang/LLVM
else ifeq ($(findstring gcc,$(COMPILER_TYPE)),gcc)
    WARNINGS := -Wall -Wno-misleading-indentation -Wno-unused-variable -Wno-unused-but-set-variable
    COMPILER_NAME := GCC (fallback)
else
    WARNINGS := -Wall
    COMPILER_NAME := Unknown
endif

# Allegro modules
ALLEGRO_MODULES := allegro-5 allegro_audio-5 allegro_acodec-5 allegro_dialog-5 allegro_font-5 allegro_image-5 allegro_primitives-5

# Build configuration
PKG_CONFIG := $(or $(PKG_CONFIG),pkg-config)
OPTIMIZATION := -O2
DEBUG_FLAGS := -g -DDEBUG_MODE
NETWORK_FLAGS := -DNETWORK_ENABLED
BASE_CFLAGS := $$($(PKG_CONFIG) --cflags $(ALLEGRO_MODULES)) $(WARNINGS) $(OPTIMIZATION)
BASE_LIBS := -lm $$($(PKG_CONFIG) --libs $(ALLEGRO_MODULES))

# Default build flags (no network)
CFLAGS := $(BASE_CFLAGS)
LIBS := $(BASE_LIBS)

# File lists
HEADERS := $(shell find src/ -name '*.h' 2>/dev/null)
SOURCES := $(shell find src/ -name '*.c' 2>/dev/null)
OBJECTS := $(SOURCES:.c=$(OBJ_EXT))
TARGET := bin/libcirc$(EXE_EXT)

# Default target
.PHONY: all clean debug network multiplayer install test info help
.DEFAULT_GOAL := all

all: $(TARGET)

# Network/Multiplayer build
network: CFLAGS := $(BASE_CFLAGS) $(NETWORK_FLAGS)
network: LIBS := $(BASE_LIBS) $(NETWORK_LIBS)
network: $(TARGET)
	@echo "✓ Network build complete with $(COMPILER_NAME)"
	@echo "  Features: LAN discovery, host/join games, lobby system"

multiplayer: network

# Debug build
debug: CFLAGS := $(BASE_CFLAGS) $(DEBUG_FLAGS)
debug: OPTIMIZATION := -O0
debug: $(TARGET)
	@echo "✓ Debug build complete with $(COMPILER_NAME)"

# Network debug build
network-debug: CFLAGS := $(BASE_CFLAGS) $(NETWORK_FLAGS) $(DEBUG_FLAGS)
network-debug: LIBS := $(BASE_LIBS) $(NETWORK_LIBS)
network-debug: OPTIMIZATION := -O0
network-debug: $(TARGET)
	@echo "✓ Network debug build complete with $(COMPILER_NAME)"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -f $(TARGET) $(OBJECTS)
	@rm -f src/*$(OBJ_EXT) src/**/*$(OBJ_EXT)
	@echo "✓ Clean complete"

# Main executable target
$(TARGET): $(OBJECTS) | bin
	@echo "Linking $(TARGET) with $(COMPILER_NAME)..."
	@$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	@echo "✓ Build complete: $(TARGET)"

# Object file compilation
%$(OBJ_EXT): %.c $(HEADERS)
	@echo "Compiling $< with $(COMPILER_NAME)..."
	@$(CC) $(CFLAGS) -c -o $@ $<

# Create bin directory
bin:
	@mkdir -p bin

# Install target (Unix-like systems only)
install: $(TARGET)
ifeq ($(PLATFORM),windows)
	@echo "Install not supported on Windows. Copy bin/ directory manually."
else
	@echo "Installing Liberation Circuit..."
	install -d $(DESTDIR)/usr/local/bin
	install -d $(DESTDIR)/usr/local/share/liberation-circuit
	install $(TARGET) $(DESTDIR)/usr/local/bin/libcirc
	cp -r bin/* $(DESTDIR)/usr/local/share/liberation-circuit/
	@echo "✓ Installation complete"
endif

# Test build
test: $(TARGET)
	@echo "Testing build..."
	@if [ -f "$(TARGET)" ]; then \
		echo "✓ Executable exists"; \
		ls -lh "$(TARGET)"; \
	else \
		echo "✗ Build failed - executable not found"; \
		exit 1; \
	fi

# Show build information
info:
	@echo "Liberation Circuit Build Information"
	@echo "===================================="
	@echo "Platform: $(PLATFORM)"
	@echo "Architecture: $(UNAME_M)"
	@echo "Compiler: $(CC) ($(COMPILER_NAME))"
	@echo "Target: $(TARGET)"
	@echo "Allegro modules: $(ALLEGRO_MODULES)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LIBS: $(LIBS)"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Standard build"
	@echo "  network    - Build with multiplayer support"  
	@echo "  debug      - Debug build"
	@echo "  clean      - Clean build artifacts"
	@echo "  install    - Install system-wide (Unix only)"
	@echo "  test       - Test the build"

# Help target
help: info

# Platform-specific targets
.PHONY: linux macos windows
linux: all
macos: all
windows: all
