#!/bin/bash

# Liberation Circuit - Automated Dependency Installation Script
# Installs all required dependencies for building Liberation Circuit

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Script metadata
SCRIPT_VERSION="1.0.0"
SCRIPT_NAME="Liberation Circuit Dependency Installer"

# Helper functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_step() {
    echo -e "${PURPLE}[STEP]${NC} $1"
}

log_command() {
    echo -e "${CYAN}[CMD]${NC} $1"
}

# Show script header
show_header() {
    echo "=================================================="
    echo "$SCRIPT_NAME v$SCRIPT_VERSION"
    echo "=================================================="
    echo ""
    echo "This script will install all dependencies needed to build"
    echo "Liberation Circuit with network/multiplayer support."
    echo ""
}

# Platform detection
detect_platform() {
    log_step "Detecting platform..."
    
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if command -v apt-get >/dev/null 2>&1; then
            PLATFORM="ubuntu"
            PACKAGE_MANAGER="apt"
            log_info "Detected Ubuntu/Debian-based system"
        elif command -v dnf >/dev/null 2>&1; then
            PLATFORM="fedora"
            PACKAGE_MANAGER="dnf"
            log_info "Detected Fedora/RHEL-based system"
        elif command -v pacman >/dev/null 2>&1; then
            PLATFORM="arch"
            PACKAGE_MANAGER="pacman"
            log_info "Detected Arch Linux-based system"
        elif command -v zypper >/dev/null 2>&1; then
            PLATFORM="opensuse"
            PACKAGE_MANAGER="zypper"
            log_info "Detected openSUSE system"
        else
            PLATFORM="linux-unknown"
            PACKAGE_MANAGER="unknown"
            log_warning "Unknown Linux distribution"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        PLATFORM="macos"
        PACKAGE_MANAGER="brew"
        log_info "Detected macOS"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        PLATFORM="windows"
        PACKAGE_MANAGER="pacman"
        log_info "Detected Windows with MSYS2"
    else
        PLATFORM="unknown"
        PACKAGE_MANAGER="unknown"
        log_error "Unsupported platform: $OSTYPE"
        exit 1
    fi
    
    echo ""
}

# Check if running as root (when needed)
check_sudo() {
    if [[ "$PLATFORM" != "macos" ]] && [[ "$PLATFORM" != "windows" ]]; then
        if [[ $EUID -eq 0 ]]; then
            log_warning "Running as root - this may not be necessary"
        elif ! command -v sudo >/dev/null 2>&1; then
            log_error "sudo is required but not available"
            log_info "Please run this script as root or install sudo"
            exit 1
        fi
    fi
}

# Update package lists
update_packages() {
    log_step "Updating package lists..."
    
    case "$PACKAGE_MANAGER" in
        "apt")
            log_command "sudo apt-get update"
            sudo apt-get update
            ;;
        "dnf")
            log_command "sudo dnf check-update"
            sudo dnf check-update || true
            ;;
        "pacman")
            if [[ "$PLATFORM" == "windows" ]]; then
                log_command "pacman -Sy"
                pacman -Sy
            else
                log_command "sudo pacman -Sy"
                sudo pacman -Sy
            fi
            ;;
        "zypper")
            log_command "sudo zypper refresh"
            sudo zypper refresh
            ;;
        "brew")
            log_command "brew update"
            brew update
            ;;
        *)
            log_warning "Unknown package manager - skipping update"
            ;;
    esac
    
    log_success "Package lists updated"
    echo ""
}

# Install build tools
install_build_tools() {
    log_step "Installing build tools..."
    
    case "$PACKAGE_MANAGER" in
        "apt")
            log_command "sudo apt-get install -y build-essential pkg-config git"
            sudo apt-get install -y build-essential pkg-config git
            ;;
        "dnf")
            log_command "sudo dnf install -y gcc gcc-c++ make pkgconfig git"
            sudo dnf install -y gcc gcc-c++ make pkgconfig git
            ;;
        "pacman")
            if [[ "$PLATFORM" == "windows" ]]; then
                log_command "pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-pkg-config git"
                pacman -S --needed \
                    mingw-w64-x86_64-gcc \
                    mingw-w64-x86_64-make \
                    mingw-w64-x86_64-pkg-config \
                    git
            else
                log_command "sudo pacman -S --needed gcc make pkgconf git"
                sudo pacman -S --needed gcc make pkgconf git
            fi
            ;;
        "zypper")
            log_command "sudo zypper install -y gcc gcc-c++ make pkg-config git"
            sudo zypper install -y gcc gcc-c++ make pkg-config git
            ;;
        "brew")
            log_command "brew install pkg-config git"
            brew install pkg-config git
            # Xcode command line tools should provide gcc
            if ! command -v gcc >/dev/null 2>&1; then
                log_info "Installing Xcode command line tools..."
                xcode-select --install || true
            fi
            ;;
    esac
    
    log_success "Build tools installed"
    echo ""
}

# Install Allegro 5
install_allegro() {
    log_step "Installing Allegro 5 and addons..."
    
    case "$PACKAGE_MANAGER" in
        "apt")
            log_command "Installing Allegro 5 packages..."
            sudo apt-get install -y \
                liballegro5-dev \
                liballegro-acodec5-dev \
                liballegro-audio5-dev \
                liballegro-dialog5-dev \
                liballegro-image5-dev \
                liballegro-primitives5-dev \
                liballegro-font5-dev \
                liballegro-ttf5-dev
            ;;
        "dnf")
            log_command "Installing Allegro 5 packages..."
            sudo dnf install -y \
                allegro5-devel \
                allegro5-addon-acodec-devel \
                allegro5-addon-audio-devel \
                allegro5-addon-dialog-devel \
                allegro5-addon-image-devel \
                allegro5-addon-primitives-devel \
                allegro5-addon-font-devel \
                allegro5-addon-ttf-devel
            ;;
        "pacman")
            if [[ "$PLATFORM" == "windows" ]]; then
                log_command "pacman -S --needed mingw-w64-x86_64-allegro"
                pacman -S --needed mingw-w64-x86_64-allegro
            else
                log_command "sudo pacman -S --needed allegro"
                sudo pacman -S --needed allegro
            fi
            ;;
        "zypper")
            log_command "Installing Allegro 5 packages..."
            sudo zypper install -y \
                allegro-devel \
                allegro-addon-acodec-devel \
                allegro-addon-audio-devel \
                allegro-addon-dialog-devel \
                allegro-addon-image-devel \
                allegro-addon-primitives-devel \
                allegro-addon-font-devel
            ;;
        "brew")
            log_command "brew install allegro"
            brew install allegro
            ;;
    esac
    
    log_success "Allegro 5 installed"
    echo ""
}

# Install additional development tools
install_dev_tools() {
    log_step "Installing optional development tools..."
    
    case "$PACKAGE_MANAGER" in
        "apt")
            # Install optional but useful tools
            sudo apt-get install -y \
                valgrind \
                gdb \
                clang-format \
                cppcheck \
                doxygen \
                2>/dev/null || log_warning "Some optional tools failed to install"
            ;;
        "dnf")
            sudo dnf install -y \
                valgrind \
                gdb \
                clang-tools-extra \
                cppcheck \
                doxygen \
                2>/dev/null || log_warning "Some optional tools failed to install"
            ;;
        "pacman")
            if [[ "$PLATFORM" == "windows" ]]; then
                pacman -S --needed \
                    mingw-w64-x86_64-gdb \
                    mingw-w64-x86_64-clang-tools-extra \
                    2>/dev/null || log_warning "Some optional tools failed to install"
            else
                sudo pacman -S --needed \
                    valgrind \
                    gdb \
                    clang \
                    cppcheck \
                    doxygen \
                    2>/dev/null || log_warning "Some optional tools failed to install"
            fi
            ;;
        "brew")
            brew install \
                llvm \
                cppcheck \
                doxygen \
                2>/dev/null || log_warning "Some optional tools failed to install"
            ;;
    esac
    
    log_success "Development tools installed"
    echo ""
}

# Install Just command runner
install_just() {
    log_step "Installing Just command runner..."
    
    if command -v just >/dev/null 2>&1; then
        log_info "Just is already installed"
        return
    fi
    
    if command -v cargo >/dev/null 2>&1; then
        log_command "cargo install just"
        cargo install just
        log_success "Just installed via Cargo"
    elif [[ "$PACKAGE_MANAGER" == "brew" ]]; then
        log_command "brew install just"
        brew install just
        log_success "Just installed via Homebrew"
    elif [[ "$PACKAGE_MANAGER" == "pacman" ]] && [[ "$PLATFORM" == "arch" ]]; then
        log_command "sudo pacman -S just"
        sudo pacman -S just
        log_success "Just installed via pacman"
    else
        log_info "Installing Just via precompiled binary..."
        
        # Download and install Just binary
        JUST_VERSION="1.14.0"
        case "$(uname -m)" in
            "x86_64")
                ARCH="x86_64"
                ;;
            "aarch64"|"arm64")
                ARCH="aarch64"
                ;;
            *)
                log_warning "Unsupported architecture for Just binary installation"
                log_info "Please install Just manually: https://github.com/casey/just"
                return
                ;;
        esac
        
        if [[ "$PLATFORM" == "windows" ]]; then
            JUST_URL="https://github.com/casey/just/releases/download/${JUST_VERSION}/just-${JUST_VERSION}-${ARCH}-pc-windows-msvc.zip"
            JUST_FILE="just.exe"
        elif [[ "$PLATFORM" == "macos" ]]; then
            JUST_URL="https://github.com/casey/just/releases/download/${JUST_VERSION}/just-${JUST_VERSION}-${ARCH}-apple-darwin.tar.gz"
            JUST_FILE="just"
        else
            JUST_URL="https://github.com/casey/just/releases/download/${JUST_VERSION}/just-${JUST_VERSION}-${ARCH}-unknown-linux-musl.tar.gz"
            JUST_FILE="just"
        fi
        
        log_command "Downloading Just from GitHub..."
        cd /tmp
        curl -L "$JUST_URL" -o just-archive
        
        if [[ "$PLATFORM" == "windows" ]]; then
            unzip just-archive
        else
            tar -xzf just-archive
        fi
        
        # Install to user's local bin
        mkdir -p "$HOME/.local/bin"
        mv "$JUST_FILE" "$HOME/.local/bin/"
        chmod +x "$HOME/.local/bin/$JUST_FILE"
        
        log_success "Just installed to $HOME/.local/bin/"
        log_info "Add $HOME/.local/bin to your PATH if it's not already there"
        
        # Clean up
        rm -f just-archive
        cd - >/dev/null
    fi
    
    echo ""
}

# Verify installation
verify_installation() {
    log_step "Verifying installation..."
    
    local all_good=true
    
    # Check build tools
    if command -v gcc >/dev/null 2>&1; then
        log_success "GCC: $(gcc --version | head -1)"
    else
        log_error "GCC not found"
        all_good=false
    fi
    
    if command -v pkg-config >/dev/null 2>&1; then
        log_success "pkg-config: $(pkg-config --version)"
    else
        log_error "pkg-config not found"
        all_good=false
    fi
    
    # Check Allegro
    local allegro_modules="allegro-5 allegro_audio-5 allegro_acodec-5 allegro_dialog-5 allegro_font-5 allegro_image-5 allegro_primitives-5"
    if pkg-config --exists $allegro_modules; then
        log_success "Allegro 5: $(pkg-config --modversion allegro-5)"
    else
        log_error "Allegro 5 or its addons not found"
        all_good=false
    fi
    
    # Check Just
    if command -v just >/dev/null 2>&1; then
        log_success "Just: $(just --version)"
    else
        log_warning "Just not found - you can still use make"
    fi
    
    echo ""
    
    if $all_good; then
        log_success "All required dependencies are installed!"
        echo ""
        echo "You can now build Liberation Circuit:"
        echo "  just build          # Build with Just"
        echo "  just network-build  # Build with network support"
        echo "  make                # Build with make"
        echo ""
        echo "To test your setup:"
        echo "  ./scripts/test-network.sh"
    else
        log_error "Some dependencies are missing. Please check the errors above."
        exit 1
    fi
}

# Setup environment hints
show_environment_hints() {
    log_step "Environment setup hints..."
    
    case "$PLATFORM" in
        "windows")
            echo "Windows (MSYS2) specific notes:"
            echo "- Always use MSYS2 MinGW64 terminal for building"
            echo "- Add MSYS2 to Windows PATH if needed"
            echo "- Check Windows Defender firewall for network features"
            ;;
        "macos")
            echo "macOS specific notes:"
            echo "- You may need to install Xcode command line tools"
            echo "- If you encounter permission issues, check System Preferences > Security"
            echo "- For Retina displays, edit bin/init.txt and set double_fonts=1"
            ;;
        "ubuntu"|"fedora"|"arch")
            echo "Linux specific notes:"
            echo "- For network features, ensure ports 7777-7778 are open"
            echo "- You may need to adjust firewall settings (ufw, iptables, etc.)"
            echo "- Run 'just setup' or this script again if you switch distributions"
            ;;
    esac
    
    echo ""
    echo "General notes:"
    echo "- Add ~/.local/bin to PATH if Just was installed there"
    echo "- Restart terminal or run 'source ~/.bashrc' to update PATH"
    echo "- Check the README.md for detailed build instructions"
    echo ""
}

# Handle script arguments
show_help() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --help, -h         Show this help message"
    echo "  --no-optional      Skip optional development tools"
    echo "  --no-just          Skip Just installation"
    echo "  --quiet, -q        Minimal output"
    echo "  --verbose, -v      Verbose output"
    echo "  --check-only       Only check existing installation"
    echo ""
    echo "Examples:"
    echo "  $0                 # Full installation"
    echo "  $0 --no-optional   # Install only required packages"
    echo "  $0 --check-only    # Check what's already installed"
    echo ""
}

# Parse command line arguments
INSTALL_OPTIONAL=true
INSTALL_JUST=true
QUIET_MODE=false
VERBOSE_MODE=false
CHECK_ONLY=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --help|-h)
            show_help
            exit 0
            ;;
        --no-optional)
            INSTALL_OPTIONAL=false
            shift
            ;;
        --no-just)
            INSTALL_JUST=false
            shift
            ;;
        --quiet|-q)
            QUIET_MODE=true
            shift
            ;;
        --verbose|-v)
            VERBOSE_MODE=true
            set -x
            shift
            ;;
        --check-only)
            CHECK_ONLY=true
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Main execution
main() {
    if [[ "$QUIET_MODE" != "true" ]]; then
        show_header
    fi
    
    detect_platform
    check_sudo
    
    if [[ "$CHECK_ONLY" == "true" ]]; then
        verify_installation
        exit 0
    fi
    
    # Confirm installation
    if [[ "$QUIET_MODE" != "true" ]]; then
        echo "This will install dependencies for Liberation Circuit on $PLATFORM"
        echo "using $PACKAGE_MANAGER package manager."
        echo ""
        read -p "Continue? [Y/n] " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Nn]$ ]]; then
            log_info "Installation cancelled by user"
            exit 0
        fi
        echo ""
    fi
    
    # Run installation steps
    update_packages
    install_build_tools
    install_allegro
    
    if [[ "$INSTALL_OPTIONAL" == "true" ]]; then
        install_dev_tools
    fi
    
    if [[ "$INSTALL_JUST" == "true" ]]; then
        install_just
    fi
    
    verify_installation
    
    if [[ "$QUIET_MODE" != "true" ]]; then
        show_environment_hints
    fi
    
    log_success "Installation complete!"
}

# Run main function
main "$@"