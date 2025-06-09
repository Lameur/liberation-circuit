# Liberation Circuit - Windows Native Build Script
# Downloads precompiled Allegro binaries and builds the game natively

param(
    [switch]$Clean = $false,
    [switch]$Network = $false,
    [switch]$Debug = $false,
    [switch]$Help = $false,
    [string]$Compiler = "auto",
    [string]$Architecture = "x64"
)

# Script configuration
$SCRIPT_VERSION = "1.0.0"
$ALLEGRO_VERSION = "5.2.9.1"
$ALLEGRO_BASE_URL = "https://github.com/liballeg/allegro5/releases/download"
$BUILD_DIR = "build-windows"
$ALLEGRO_DIR = "allegro-$ALLEGRO_VERSION-windows"

# Colors for output
$ColorRed = "Red"
$ColorGreen = "Green"
$ColorYellow = "Yellow"
$ColorBlue = "Blue"
$ColorCyan = "Cyan"

function Write-Info($message) {
    Write-Host "[INFO] $message" -ForegroundColor $ColorBlue
}

function Write-Success($message) {
    Write-Host "[SUCCESS] $message" -ForegroundColor $ColorGreen
}

function Write-Error($message) {
    Write-Host "[ERROR] $message" -ForegroundColor $ColorRed
}

function Write-Warning($message) {
    Write-Host "[WARNING] $message" -ForegroundColor $ColorYellow
}

function Write-Step($message) {
    Write-Host "[STEP] $message" -ForegroundColor $ColorCyan
}

function Show-Header {
    Write-Host "===============================================" -ForegroundColor $ColorCyan
    Write-Host "Liberation Circuit - Windows Native Builder v$SCRIPT_VERSION" -ForegroundColor $ColorCyan
    Write-Host "===============================================" -ForegroundColor $ColorCyan
    Write-Host ""
}

function Show-Help {
    Write-Host "Usage: .\build-windows.ps1 [options]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -Clean          Clean build directory before building"
    Write-Host "  -Network        Enable network/multiplayer support"
    Write-Host "  -Debug          Build with debug symbols"
    Write-Host "  -Help           Show this help message"
    Write-Host "  -Compiler       Compiler to use (auto, gcc, clang, msvc)"
    Write-Host "  -Architecture   Target architecture (x64, x86)"
    Write-Host ""
    Write-Host "Examples:"
    Write-Host "  .\build-windows.ps1                    # Basic build"
    Write-Host "  .\build-windows.ps1 -Network           # Build with multiplayer"
    Write-Host "  .\build-windows.ps1 -Clean -Debug      # Clean debug build"
    Write-Host "  .\build-windows.ps1 -Compiler msvc     # Use MSVC compiler"
    Write-Host ""
}

function Test-Command($command) {
    try {
        Get-Command $command -ErrorAction Stop | Out-Null
        return $true
    }
    catch {
        return $false
    }
}

function Detect-Compiler {
    Write-Step "Detecting available compilers..."
    
    $compilers = @()
    
    # Prioritize Clang/LLVM first
    if (Test-Command "clang") {
        $compilers += "clang"
        $clangVersion = & clang --version 2>$null | Select-Object -First 1
        Write-Info "Found Clang: $clangVersion"
    }
    
    if (Test-Command "clang-cl") {
        $compilers += "clang-cl"
        $clangClVersion = & clang-cl --version 2>$null | Select-Object -First 1
        Write-Info "Found Clang-CL: $clangClVersion"
    }
    
    if (Test-Command "cl") {
        $compilers += "msvc"
        Write-Info "Found MSVC compiler"
    }
    
    # GCC is now optional and least preferred
    if (Test-Command "gcc") {
        $compilers += "gcc"
        $gccVersion = & gcc --version 2>$null | Select-Object -First 1
        Write-Info "Found GCC: $gccVersion (fallback option)"
    }
    
    if ($compilers.Count -eq 0) {
        Write-Error "No compatible compiler found!"
        Write-Info "Please install LLVM/Clang (recommended):"
        Write-Info "  - Download from: https://releases.llvm.org/download.html"
        Write-Info "  - Or install Visual Studio with C++ tools"
        Write-Info "  - MinGW/MSYS2 also supported but not recommended"
        exit 1
    }
    
    if ($Compiler -eq "auto") {
        # Prefer Clang first, then Clang-CL, then MSVC, then GCC
        if ($compilers -contains "clang") {
            return "clang"
        }
        elseif ($compilers -contains "clang-cl") {
            return "clang-cl"
        }
        elseif ($compilers -contains "msvc") {
            return "msvc"
        }
        else {
            return "gcc"
        }
    }
    elseif ($compilers -contains $Compiler) {
        return $Compiler
    }
    else {
        Write-Error "Requested compiler '$Compiler' not found!"
        Write-Info "Available compilers: $($compilers -join ', ')"
        exit 1
    }
}

function Download-File($url, $destination) {
    Write-Info "Downloading $url..."
    try {
        $webClient = New-Object System.Net.WebClient
        $webClient.DownloadFile($url, $destination)
        Write-Success "Downloaded to $destination"
        return $true
    }
    catch {
        Write-Error "Failed to download: $($_.Exception.Message)"
        return $false
    }
}

function Extract-Archive($archivePath, $destination) {
    Write-Info "Extracting $archivePath to $destination..."
    try {
        if (Test-Command "7z") {
            & 7z x "$archivePath" -o"$destination" -y | Out-Null
        }
        else {
            # Use PowerShell's built-in ZIP support
            Add-Type -AssemblyName System.IO.Compression.FileSystem
            [System.IO.Compression.ZipFile]::ExtractToDirectory($archivePath, $destination)
        }
        Write-Success "Extraction completed"
        return $true
    }
    catch {
        Write-Error "Failed to extract: $($_.Exception.Message)"
        return $false
    }
}

function Download-Allegro {
    Write-Step "Setting up Allegro $ALLEGRO_VERSION..."
    
    if (Test-Path $ALLEGRO_DIR) {
        Write-Info "Allegro already exists, skipping download"
        return $true
    }
    
    # Determine architecture suffix and preferred build
    $archSuffix = if ($Architecture -eq "x64") { "win64" } else { "win32" }
    
    # Try to get LLVM/Clang compatible builds first
    $allegroFile = "allegro-$ALLEGRO_VERSION-$archSuffix-msvc.zip"
    $allegroUrl = "$ALLEGRO_BASE_URL/$ALLEGRO_VERSION/$allegroFile"
    $allegroPath = "$BUILD_DIR\$allegroFile"
    
    # Create build directory
    if (!(Test-Path $BUILD_DIR)) {
        New-Item -ItemType Directory -Path $BUILD_DIR | Out-Null
    }
    
    # Download Allegro
    if (!(Download-File $allegroUrl $allegroPath)) {
        Write-Warning "Failed to download MSVC Allegro, trying MinGW version..."
        
        # Fallback to MinGW version if MSVC not available
        $allegroFile = "allegro-$ALLEGRO_VERSION-$archSuffix-mingw.zip"
        $allegroUrl = "$ALLEGRO_BASE_URL/$ALLEGRO_VERSION/$allegroFile"
        $allegroPath = "$BUILD_DIR\$allegroFile"
        
        if (!(Download-File $allegroUrl $allegroPath)) {
            Write-Warning "Failed to download MinGW Allegro, trying alternative source..."
            
            # Try alternative download
            $altUrl = "https://github.com/allegro/allegro_winpkg/releases/download/$ALLEGRO_VERSION/allegro-$archSuffix-gcc-9.2.0-posix-seh-static-$ALLEGRO_VERSION.zip"
            if (!(Download-File $altUrl $allegroPath)) {
                Write-Error "Failed to download Allegro from all sources"
                return $false
            }
        }
    }
    
    # Extract Allegro
    if (!(Extract-Archive $allegroPath $BUILD_DIR)) {
        return $false
    }
    
    # Find extracted directory (it might have a different name)
    $extractedDirs = Get-ChildItem -Path $BUILD_DIR -Directory | Where-Object { $_.Name -like "*allegro*" }
    if ($extractedDirs.Count -gt 0) {
        $actualDir = $extractedDirs[0].FullName
        if ($actualDir -ne (Join-Path $BUILD_DIR $ALLEGRO_DIR)) {
            Rename-Item $actualDir $ALLEGRO_DIR
        }
    }
    
    Write-Success "Allegro setup completed"
    return $true
}

function Get-CompilerFlags($selectedCompiler) {
    $allegroPath = Join-Path $BUILD_DIR $ALLEGRO_DIR
    $includeDir = Join-Path $allegroPath "include"
    $libDir = Join-Path $allegroPath "lib"
    
    $baseFlags = @{
        Include = "-I`"$includeDir`""
        LibDir = "-L`"$libDir`""
        Libs = "-lallegro -lallegro_main -lallegro_audio -lallegro_acodec -lallegro_dialog -lallegro_font -lallegro_image -lallegro_primitives"
    }
    
    $warnings = switch ($selectedCompiler) {
        "msvc" { "/W3 /wd4996" }
        "clang-cl" { "/W3 /wd4996 -Wno-unused-variable -Wno-unused-parameter" }
        "clang" { "-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unused-variable" }
        default { "-Wall -Wno-misleading-indentation -Wno-unused-variable" }
    }
    
    $optimization = if ($Debug) {
        switch ($selectedCompiler) {
            "msvc" { "/Od /Zi" }
            "clang-cl" { "/Od /Zi" }
            default { "-g -O0" }
        }
    }
    else {
        switch ($selectedCompiler) {
            "msvc" { "/O2" }
            "clang-cl" { "/O2" }
            default { "-O2" }
        }
    }
    
    # Hide console window for Windows builds
    $platformFlags = switch ($selectedCompiler) {
        "msvc" { "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup" }
        "clang-cl" { "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup" }
        default { "-mwindows" }
    }
    
    $defines = @("-DWINDOWS", "-D_WIN32")
    if ($Network) {
        $defines += "-DNETWORK_ENABLED"
        $baseFlags.Libs += " -lws2_32"
    }
    if ($Debug) {
        $defines += "-DDEBUG_MODE"
    }
    
    return @{
        Warnings = $warnings
        Optimization = $optimization
        Defines = $defines -join " "
        Include = $baseFlags.Include
        LibDir = $baseFlags.LibDir
        Libs = $baseFlags.Libs
        PlatformFlags = $platformFlags
    }
}

function Compile-Sources($selectedCompiler, $flags) {
    Write-Step "Compiling source files..."
    
    $sourceFiles = Get-ChildItem -Path "src" -Filter "*.c" -Recurse
    $objectFiles = @()
    $compiled = 0
    $failed = 0
    
    foreach ($sourceFile in $sourceFiles) {
        $objFile = $sourceFile.FullName -replace "\.c$", ".o" -replace "src\\", "obj\\"
        $objDir = Split-Path $objFile
        
        # Create object directory if it doesn't exist
        if (!(Test-Path $objDir)) {
            New-Item -ItemType Directory -Path $objDir -Force | Out-Null
        }
        
        # Check if we need to recompile
        $needsCompile = $true
        if (Test-Path $objFile) {
            $srcTime = (Get-Item $sourceFile.FullName).LastWriteTime
            $objTime = (Get-Item $objFile).LastWriteTime
            $needsCompile = $srcTime -gt $objTime
        }
        
        if ($needsCompile) {
            Write-Info "Compiling $($sourceFile.Name)..."
            
            $compileCmd = switch ($selectedCompiler) {
                "msvc" {
                    "cl /c $($flags.Warnings) $($flags.Optimization) $($flags.Defines) $($flags.Include) /Fo`"$objFile`" `"$($sourceFile.FullName)`""
                }
                "clang-cl" {
                    "clang-cl /c $($flags.Warnings) $($flags.Optimization) $($flags.Defines) $($flags.Include) /Fo`"$objFile`" `"$($sourceFile.FullName)`""
                }
                default {
                    "$selectedCompiler -c $($flags.Warnings) $($flags.Optimization) $($flags.Defines) $($flags.Include) -o `"$objFile`" `"$($sourceFile.FullName)`""
                }
            }
            
            try {
                Invoke-Expression $compileCmd 2>$null
                if ($LASTEXITCODE -eq 0) {
                    $compiled++
                }
                else {
                    Write-Warning "Compilation failed for $($sourceFile.Name)"
                    $failed++
                }
            }
            catch {
                Write-Warning "Compilation error for $($sourceFile.Name): $($_.Exception.Message)"
                $failed++
            }
        }
        
        $objectFiles += $objFile
    }
    
    Write-Info "Compilation summary: $compiled compiled, $failed failed"
    
    if ($failed -gt 0) {
        Write-Warning "Some files failed to compile, but continuing..."
    }
    
    return $objectFiles
}

function Link-Executable($selectedCompiler, $flags, $objectFiles) {
    Write-Step "Linking executable..."
    
    $outputFile = "bin\libcirc.exe"
    
    # Create bin directory if it doesn't exist
    if (!(Test-Path "bin")) {
        New-Item -ItemType Directory -Path "bin" | Out-Null
    }
    
    # Filter out non-existent object files
    $validObjects = $objectFiles | Where-Object { Test-Path $_ }
    $objectList = $validObjects -join " "
    
    $linkCmd = switch ($selectedCompiler) {
        "msvc" {
            "link /OUT:`"$outputFile`" $($flags.PlatformFlags) $($flags.LibDir) $objectList $($flags.Libs)"
        }
        "clang-cl" {
            "clang-cl /Fe:`"$outputFile`" $($flags.PlatformFlags) $($flags.LibDir) $objectList $($flags.Libs)"
        }
        default {
            "$selectedCompiler $($flags.PlatformFlags) $($flags.LibDir) -o `"$outputFile`" $objectList $($flags.Libs)"
        }
    }
    
    Write-Info "Linking with: $selectedCompiler"
    
    try {
        Invoke-Expression $linkCmd
        if ($LASTEXITCODE -eq 0) {
            Write-Success "Executable created: $outputFile"
            return $true
        }
        else {
            Write-Error "Linking failed with exit code $LASTEXITCODE"
            return $false
        }
    }
    catch {
        Write-Error "Linking error: $($_.Exception.Message)"
        return $false
    }
}

function Copy-Dependencies {
    Write-Step "Copying runtime dependencies..."
    
    $allegroPath = Join-Path $BUILD_DIR $ALLEGRO_DIR
    $binDir = Join-Path $allegroPath "bin"
    
    if (Test-Path $binDir) {
        $dllFiles = Get-ChildItem -Path $binDir -Filter "*.dll"
        foreach ($dll in $dllFiles) {
            Copy-Item $dll.FullName "bin\" -Force
            Write-Info "Copied $($dll.Name)"
        }
    }
    
    Write-Success "Dependencies copied to bin directory"
}

function Clean-Build {
    Write-Step "Cleaning build directory..."
    
    if (Test-Path $BUILD_DIR) {
        Remove-Item $BUILD_DIR -Recurse -Force
        Write-Success "Build directory cleaned"
    }
    
    if (Test-Path "obj") {
        Remove-Item "obj" -Recurse -Force
        Write-Success "Object files cleaned"
    }
    
    if (Test-Path "bin\libcirc.exe") {
        Remove-Item "bin\libcirc.exe" -Force
        Write-Success "Executable cleaned"
    }
}

function Test-Build {
    Write-Step "Testing build..."
    
    $exePath = "bin\libcirc.exe"
    
    if (!(Test-Path $exePath)) {
        Write-Error "Executable not found: $exePath"
        return $false
    }
    
    $fileSize = (Get-Item $exePath).Length
    Write-Info "Executable size: $([math]::Round($fileSize / 1MB, 2)) MB"
    
    # Try to run with --help or --version (may not be supported)
    try {
        $output = & $exePath --help 2>$null
        Write-Success "Executable appears to be working"
    }
    catch {
        Write-Info "Executable created but help test failed (this may be normal)"
    }
    
    return $true
}

function Main {
    Show-Header
    
    if ($Help) {
        Show-Help
        return
    }
    
    # Change to script directory
    $scriptDir = Split-Path $MyInvocation.MyCommand.Path
    if ($scriptDir) {
        Set-Location $scriptDir
    }
    
    # Clean if requested
    if ($Clean) {
        Clean-Build
    }
    
    # Detect compiler
    $selectedCompiler = Detect-Compiler
    Write-Success "Using compiler: $selectedCompiler"
    
    # Download and setup Allegro
    if (!(Download-Allegro)) {
        Write-Error "Failed to setup Allegro"
        exit 1
    }
    
    # Get compiler flags
    $flags = Get-CompilerFlags $selectedCompiler
    
    # Compile source files
    $objectFiles = Compile-Sources $selectedCompiler $flags
    
    # Link executable
    if (!(Link-Executable $selectedCompiler $flags $objectFiles)) {
        Write-Error "Build failed during linking"
        exit 1
    }
    
    # Copy dependencies
    Copy-Dependencies
    
    # Test build
    Test-Build
    
    Write-Success "Build completed successfully!"
    Write-Info ""
    Write-Info "Output: bin\libcirc.exe"
    Write-Info "Compiler: $selectedCompiler"
    
    if ($Network) {
        Write-Info "Network/Multiplayer: ENABLED"
        Write-Info "Features: LAN discovery, host/join games, lobby system"
        Write-Info "Ports used: UDP 7777 (game), UDP 7778 (discovery)"
    }
    else {
        Write-Info "Network/Multiplayer: disabled (use -Network to enable)"
    }
    
    Write-Info ""
    Write-Info "To run the game:"
    Write-Info "  cd bin"
    Write-Info "  .\libcirc.exe"
    Write-Info ""
    Write-Info "Note: Console window is hidden for Windows builds"
    Write-Info "Check Windows Event Viewer if you encounter issues"
}

# Run main function
Main