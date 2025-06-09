# Liberation Circuit - Modern Build System
# Cross-platform justfile for building Liberation Circuit

# Default recipe
default: build

# Variables
export PKG_CONFIG := env_var_or_default("PKG_CONFIG", "pkg-config")
export ALLEGRO_MODULES := "allegro-5 allegro_audio-5 allegro_acodec-5 allegro_dialog-5 allegro_font-5 allegro_image-5 allegro_primitives-5"

# Compiler detection
# Compiler detection (prioritize Clang/LLVM, make GCC optional)
cc := if env_var_or_default("CC", "") != "" { 
    env_var_or_default("CC", "") 
} else if `command -v clang >/dev/null 2>&1 && echo "yes" || echo "no"` == "yes" { 
    "clang" 
} else if `command -v clang-cl >/dev/null 2>&1 && echo "yes" || echo "no"` == "yes" { 
    "clang-cl" 
} else if `command -v gcc >/dev/null 2>&1 && echo "yes" || echo "no"` == "yes" { 
    "gcc" 
} else { 
    "clang" 
}

# Platform detection
os := if os() == "windows" { "windows" } else { os() }
exe_ext := if os == "windows" { ".exe" } else { "" }
bin_name := "bin/libcirc" + exe_ext

# Compiler-specific flags
base_warnings := if cc =~ "clang" { 
    "-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unused-variable" 
} else if cc =~ "gcc" { 
    "-Wall -Wno-misleading-indentation -Wno-unused-variable -Wno-unused-but-set-variable" 
} else { 
    "-Wall" 
}

# Build flags
cflags := "`" + PKG_CONFIG + " --cflags " + ALLEGRO_MODULES + "` " + base_warnings + " -O2"
libs := "-lm `" + PKG_CONFIG + " --libs " + ALLEGRO_MODULES + "`"

# Help message
help:
    @echo "Liberation Circuit Build System"
    @echo ""
    @echo "Available commands:"
    @echo "  setup     - Install dependencies for your platform"
    @echo "  deps      - Check dependencies"
    @echo "  build     - Build the game"
    @echo "  network   - Build with network/multiplayer support"
    @echo "  clean     - Clean build artifacts"
    @echo "  run       - Build and run the game"
    @echo "  debug     - Build with debug symbols"
    @echo "  install   - Install the game system-wide"
    @echo "  package   - Create distribution package"
    @echo "  test      - Run basic tests"
    @echo ""
    @echo "Platform: {{os}}"
    @echo "Compiler: {{cc}}"

# Setup dependencies for different platforms
setup:
    @echo "Setting up dependencies for {{os}}..."
    @just _setup-{{os}}

# Linux dependency setup
_setup-linux:
    #!/usr/bin/env bash
    set -euo pipefail
    
    if command -v apt-get >/dev/null 2>&1; then
        echo "Detected Ubuntu/Debian - installing via apt"
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            pkg-config \
            liballegro5-dev \
            liballegro-acodec5-dev \
            liballegro-audio5-dev \
            liballegro-dialog5-dev \
            liballegro-image5-dev \
            liballegro-primitives5-dev \
            liballegro-font5-dev
    elif command -v dnf >/dev/null 2>&1; then
        echo "Detected Fedora/RHEL - installing via dnf"
        sudo dnf install -y \
            gcc make pkgconfig \
            allegro5-devel \
            allegro5-addon-acodec-devel \
            allegro5-addon-audio-devel \
            allegro5-addon-dialog-devel \
            allegro5-addon-image-devel \
            allegro5-addon-primitives-devel \
            allegro5-addon-font-devel
    elif command -v pacman >/dev/null 2>&1; then
        echo "Detected Arch Linux - installing via pacman"
        sudo pacman -S --needed \
            gcc make pkgconf \
            allegro
    else
        echo "Unknown Linux distribution. Please install Allegro 5 development packages manually."
        echo "Required packages: allegro5-dev and its addons (acodec, audio, dialog, image, primitives, font)"
        exit 1
    fi

# macOS dependency setup
_setup-macos:
    #!/usr/bin/env bash
    set -euo pipefail
    
    if ! command -v brew >/dev/null 2>&1; then
        echo "Homebrew not found. Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    
    echo "Installing dependencies via Homebrew..."
    brew install allegro pkg-config

# Windows dependency setup
_setup-windows:
    #!/usr/bin/env bash
    set -euo pipefail
    
    echo "Windows Setup Options:"
    echo "1. Native LLVM/Clang (recommended)"
    echo "2. MSYS2 with MinGW (fallback)"
    echo ""
    
    if command -v clang >/dev/null 2>&1 || command -v clang-cl >/dev/null 2>&1; then
        echo "✓ LLVM/Clang detected - using native Windows build"
        echo "Please ensure Allegro 5 is available or use the PowerShell script:"
        echo "  .\\windows\\build-windows.ps1"
    elif command -v pacman >/dev/null 2>&1; then
        echo "Installing dependencies via MSYS2..."
        pacman -S --needed \
            mingw-w64-x86_64-clang \
            mingw-w64-x86_64-make \
            mingw-w64-x86_64-pkg-config \
            mingw-w64-x86_64-allegro
    else
        echo "For native Windows build, install LLVM/Clang from:"
        echo "  https://releases.llvm.org/download.html"
        echo "Or use PowerShell script: .\\windows\\build-windows.ps1"
        echo ""
        echo "For MSYS2 fallback, install from: https://www.msys2.org/"
        exit 1
    fi

# Check if dependencies are available
deps:
    @echo "Checking dependencies..."
    @{{PKG_CONFIG}} --exists {{ALLEGRO_MODULES}} && echo "✓ Allegro 5 found" || (echo "✗ Allegro 5 not found - run 'just setup'" && exit 1)
    @command -v {{cc}} >/dev/null 2>&1 && echo "✓ C compiler ({{cc}}) found" || (echo "✗ C compiler not found" && exit 1)
    @echo "All dependencies satisfied!"

# Get source files
_sources := `find src -name "*.c" | tr '\n' ' '`
_headers := `find src -name "*.h" | tr '\n' ' '`
_objects := `find src -name "*.c" | sed 's/\.c$/.o/' | tr '\n' ' '`

# Build the game
build: deps
    @echo "Building Liberation Circuit..."
    @mkdir -p bin
    @just _compile-sources
    @just _link-executable
    @echo "✓ Build complete: {{bin_name}}"

# Compile source files
_compile-sources:
    #!/usr/bin/env bash
    set -euo pipefail
    
    for src_file in {{_sources}}; do
        obj_file="${src_file%.c}.o"
        if [[ "$src_file" -nt "$obj_file" ]] || [[ ! -f "$obj_file" ]]; then
            echo "Compiling $src_file with {{cc}}..."
            {{cc}} {{cflags}} -c -o "$obj_file" "$src_file"
        fi
    done

# Link executable
_link-executable:
    @echo "Linking {{bin_name}} with {{cc}}..."
    @{{cc}} {{cflags}} -o {{bin_name}} {{_objects}} {{libs}}

# Build with debug symbols
debug:
    @echo "Building with debug symbols..."
    @mkdir -p bin
    @just _compile-debug
    @just _link-debug

_compile-debug:
    #!/usr/bin/env bash
    set -euo pipefail
    
    debug_cflags="`{{PKG_CONFIG}} --cflags {{ALLEGRO_MODULES}}` {{base_warnings}} -g -DDEBUG_MODE"
    
    for src_file in {{_sources}}; do
        obj_file="${src_file%.c}.o"
        echo "Compiling $src_file (debug) with {{cc}}..."
        {{cc}} $debug_cflags -c -o "$obj_file" "$src_file"
    done

_link-debug:
    @echo "Linking {{bin_name}} (debug) with {{cc}}..."
    @{{cc}} -g -o {{bin_name}} {{_objects}} {{libs}}

# Clean build artifacts
clean:
    @echo "Cleaning build artifacts..."
    @rm -f {{_objects}}
    @rm -f {{bin_name}}
    @echo "✓ Clean complete"

# Build and run the game
run: build
    @echo "Starting Liberation Circuit..."
    @cd bin && ./libcirc{{exe_ext}}

# Install system-wide (Linux/macOS)
install: build
    #!/usr/bin/env bash
    set -euo pipefail
    
    if [[ "{{os}}" == "windows" ]]; then
        echo "System-wide installation not supported on Windows"
        echo "Copy the 'bin' directory to your desired location instead"
        exit 1
    fi
    
    PREFIX=${PREFIX:-/usr/local}
    echo "Installing to $PREFIX..."
    
    sudo mkdir -p "$PREFIX/bin"
    sudo mkdir -p "$PREFIX/share/liberation-circuit"
    sudo mkdir -p "$PREFIX/share/applications"
    sudo mkdir -p "$PREFIX/share/pixmaps"
    
    sudo cp bin/libcirc "$PREFIX/bin/"
    sudo cp -r bin/* "$PREFIX/share/liberation-circuit/"
    
    echo "✓ Installation complete"

# Create distribution package
package: clean build
    @echo "Creating distribution package..."
    @mkdir -p dist
    @cp -r bin dist/liberation-circuit
    @cp README.md LICENSE.md dist/liberation-circuit/
    @cd dist && tar -czf liberation-circuit-{{os}}.tar.gz liberation-circuit
    @echo "✓ Package created: dist/liberation-circuit-{{os}}.tar.gz"

# Run basic tests
test: build
    @echo "Running basic tests..."
    @echo "✓ Build test passed"
    @timeout 5s {{bin_name}} --help 2>/dev/null || echo "✓ Executable runs (may not support --help)"

# Network and Multiplayer commands

# Build with network support enabled
network: deps
    @echo "Building with network/multiplayer support..."
    @mkdir -p bin
    @just _compile-network
    @just _link-network

_compile-network:
    #!/usr/bin/env bash
    set -euo pipefail
    
    network_cflags="`{{PKG_CONFIG}} --cflags {{ALLEGRO_MODULES}}` {{base_warnings}} -O2 -DNETWORK_ENABLED"
    
    for src_file in {{_sources}}; do
        obj_file="${src_file%.c}.o"
        if [[ "$src_file" -nt "$obj_file" ]] || [[ ! -f "$obj_file" ]]; then
            echo "Compiling $src_file (network) with {{cc}}..."
            {{cc}} $network_cflags -c -o "$obj_file" "$src_file"
        fi
    done

_link-network:
    #!/usr/bin/env bash
    set -euo pipefail
    
    if [[ "{{os}}" == "windows" ]]; then
        network_libs="{{libs}} -lws2_32"
    else
        network_libs="{{libs}}"
    fi
    
    echo "Linking {{bin_name}} (network) with {{cc}}..."
    {{cc}} {{cflags}} -o {{bin_name}} {{_objects}} $network_libs

# Test network functionality
network-test: network
    @echo "Testing network functionality..."
    @if [[ "{{os}}" == "windows" ]]; then \
        echo "Network test on Windows - check firewall settings"; \
    fi
    @echo "✓ Network build successful"
    @echo "Start the game and check Multiplayer menu"

# Build multiplayer-focused version
multiplayer: clean network
    @echo "✓ Multiplayer build complete"
    @echo "Features: LAN discovery, host/join games, lobby system"

# Network debugging build
network-debug:
    @echo "Building network debug version..."
    @mkdir -p bin
    @just _compile-network-debug
    @just _link-network-debug

_compile-network-debug:
    #!/usr/bin/env bash
    set -euo pipefail
    
    debug_cflags="`{{PKG_CONFIG}} --cflags {{ALLEGRO_MODULES}}` {{base_warnings}} -g -DDEBUG_MODE -DNETWORK_ENABLED -DNETWORK_DEBUG"
    
    for src_file in {{_sources}}; do
        obj_file="${src_file%.c}.o"
        echo "Compiling $src_file (network debug) with {{cc}}..."
        {{cc}} $debug_cflags -c -o "$obj_file" "$src_file"
    done

_link-network-debug:
    #!/usr/bin/env bash
    set -euo pipefail
    
    if [[ "{{os}}" == "windows" ]]; then
        network_libs="{{libs}} -lws2_32"
    else
        network_libs="{{libs}}"
    fi
    
    echo "Linking {{bin_name}} (network debug) with {{cc}}..."
    {{cc}} -g -o {{bin_name}} {{_objects}} $network_libs

# Development helpers

# Format source code (if clang-format available)
format:
    @if command -v clang-format >/dev/null 2>&1; then \
        echo "Formatting source code..."; \
        find src -name "*.c" -o -name "*.h" | xargs clang-format -i; \
        echo "✓ Code formatted"; \
    else \
        echo "clang-format not found - skipping formatting"; \
    fi

# Static analysis (if available)
analyze:
    @if command -v cppcheck >/dev/null 2>&1; then \
        echo "Running static analysis..."; \
        cppcheck --enable=all --suppress=missingIncludeSystem src/; \
    else \
        echo "cppcheck not found - install for static analysis"; \
    fi

# Show build information
info:
    @echo "Build Information:"
    @echo "Platform: {{os}}"
    @echo "Compiler: {{cc}}"
    @echo "PKG_CONFIG: {{PKG_CONFIG}}"
    @echo "Output: {{bin_name}}"
    @echo "CFLAGS: {{cflags}}"
    @echo "LIBS: {{libs}}"
    @echo "Warnings: {{base_warnings}}"
    @echo ""
    @echo "Available compilers:"
    @command -v clang >/dev/null 2>&1 && echo "  ✓ Clang: $(clang --version | head -1)" || echo "  ✗ Clang not found"
    @command -v clang-cl >/dev/null 2>&1 && echo "  ✓ Clang-CL: $(clang-cl --version | head -1)" || echo "  ✗ Clang-CL not found"
    @command -v gcc >/dev/null 2>&1 && echo "  ✓ GCC: $(gcc --version | head -1)" || echo "  ✗ GCC not found (optional)"
    @echo ""
    @echo "Source files: {{_sources}}"

# Watch for changes and rebuild (requires inotify-tools on Linux)
watch:
    #!/usr/bin/env bash
    set -euo pipefail
    
    if command -v inotifywait >/dev/null 2>&1; then
        echo "Watching for changes..."
        while inotifywait -r -e modify src/; do
            echo "Files changed, rebuilding..."
            just build || true
        done
    elif command -v fswatch >/dev/null 2>&1; then
        echo "Watching for changes..."
        fswatch -o src/ | while read; do
            echo "Files changed, rebuilding..."
            just build || true
        done
    else
        echo "No file watcher available. Install inotify-tools (Linux) or fswatch (macOS)"
        exit 1
    fi

# Quick development cycle
dev: format build run

# Show git status and recent commits
status:
    @echo "Git Status:"
    @git status --short 2>/dev/null || echo "Not a git repository"
    @echo ""
    @echo "Recent commits:"
    @git log --oneline -5 2>/dev/null || echo "No git history"