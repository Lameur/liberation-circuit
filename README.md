# Liberation Circuit
> [!notice] This is a fork

This is the release version of Liberation Circuit, an RTS/programming
game.

To play the pre-built binaries on Windows, [download the latest
release](https://github.com/linleyh/liberation-circuit/releases) and run
`LibCirc.exe`. There are also pre-built binaries for all platforms
available on [itch.io](https://linleyh.itch.io/liberation-circuit).

Vanilla has set up a [Discord server](https://discord.gg/8q7DFaM) to
discuss strategy and things.

## Screenshots

![a screenshot](https://i.imgur.com/pPIJ03I.png)
![another screenshot](https://i.imgur.com/QKWzkqA.png)

## Quick Start

### Prerequisites

- A C compiler (GCC, Clang, or MSVC)
- [Just](https://github.com/casey/just) command runner
- Git (optional, for development)

### Build and Run

```sh
git clone https://github.com/linleyh/liberation-circuit.git
cd liberation-circuit

# Install dependencies automatically
just setup

# Build and run the game
just run
```

## Dependencies

The following libraries are required to build Liberation Circuit:

* **Allegro 5** - Main game library
* **Allegro 5 addons**: acodec, audio, dialog, font, image, primitives
* **C compiler** - GCC, Clang, or MSVC
* **pkg-config** - For finding library dependencies

All dependencies can be installed automatically with `just setup`.

## Building

Liberation Circuit uses a modern `justfile` for cross-platform building. The justfile automatically detects your platform and uses appropriate build commands.

### Available Commands

```sh
just setup     # Install dependencies for your platform
just build     # Build the game
just run       # Build and run the game
just clean     # Clean build artifacts
just debug     # Build with debug symbols
just test      # Run basic tests
just help      # Show all available commands
```

The executable will be created as `bin/libcirc` (Linux/macOS) or `bin/libcirc.exe` (Windows). The game requires write access to the `bin` directory to save mission progress.

- [Manual.html](bin/Manual.html) has extensive detail about the game, including documentation for the in-game API.
- Edit [init.txt](bin/init.txt) to set screen resolution and other options (fullscreen, sound volume, key rebinding, colourblind mode etc).

## Platform-Specific Instructions

### Linux

```sh
# Install dependencies (automatically detects your distro)
just setup

# Build and run
just run
```

Supported distributions:
- **Ubuntu/Debian**: Uses `apt-get`
- **Fedora/RHEL**: Uses `dnf`
- **Arch Linux**: Uses `pacman`

### macOS

```sh
# Install Homebrew if needed, then install dependencies
just setup

# Build and run
just run
```

If you are using a Retina screen, you may want to set the `double_fonts` option to make the text larger (edit `init.txt` to do this).

### Windows

**Native Windows builds are now supported!**

1. **Install MSYS2** from [https://www.msys2.org/](https://www.msys2.org/)
2. **Open MSYS2 MinGW64 terminal**
3. **Install Just**:
   ```sh
   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
   source ~/.cargo/env
   cargo install just
   ```
4. **Build the game**:
   ```sh
   git clone https://github.com/linleyh/liberation-circuit.git
   cd liberation-circuit
   just setup
   just run
   ```

The Windows executable will be created as `bin/libcirc.exe`.

### Alternative Build Methods

For compatibility, the original build methods are still available:

- **Make**: `make` (Linux/macOS only)
- **Redo**: `./do` or `redo` (if installed)

## Multiplayer (LAN)

Liberation Circuit now supports **LAN multiplayer** for up to 8 players! You can host and join games over your local network.

### Quick Multiplayer Setup

1. **Build with network support**:
   ```sh
   just network-build
   just run
   ```

2. **Host a game**:
   - Start the game and go to "Multiplayer" → "Host Game"
   - Enter a game name and your player name
   - Click "Start Hosting"
   - Share your IP address with other players

3. **Join a game**:
   - Go to "Multiplayer" → "Browse Games" (for auto-discovery)
   - Or go to "Join Game" and enter the host's IP address
   - Enter your player name and click "Connect"

### Network Requirements

- **Ports**: The game uses UDP ports 7777 (game) and 7778 (discovery)
- **Firewall**: Make sure these ports are open in your firewall
- **Network**: All players must be on the same local network (LAN)

### Multiplayer Features

- **Automatic game discovery** on LAN
- **Real-time synchronization** of game state
- **Player lobby** with ready status
- **In-game chat** system
- **Host migration** support
- **Cross-platform** multiplayer (Windows ↔ Linux ↔ macOS)

### Hosting a Game

```sh
# Build with network support
just multiplayer

# Or use regular build with network
just network-build && just run
```

1. Select **Multiplayer** from main menu
2. Choose **Host Game**
3. Configure:
   - **Game Name**: Visible to other players
   - **Player Name**: Your display name
   - **Max Players**: 2-8 players
   - **Port**: Default 7777 (change if needed)
4. Click **Start Hosting**
5. Wait in lobby for players to join
6. Click **Start Game** when ready

### Joining a Game

#### Option 1: Browse Games (Automatic Discovery)
1. Select **Multiplayer** → **Browse Games**
2. Click **Refresh** to scan for local games
3. Select a game from the list
4. Enter your player name
5. Click **Join Game**

#### Option 2: Direct Connection
1. Select **Multiplayer** → **Join Game**
2. Enter host's IP address and port
3. Enter your player name
4. Click **Connect**

### Network Testing

Test your network setup:

```sh
# Run network diagnostics
./scripts/test-network.sh

# Quick network test
just network-test
```

### Troubleshooting

**Can't see games in browser?**
- Check firewall settings (ports 7777 and 7778)
- Ensure all players are on same network
- Try direct IP connection instead

**Connection timeout?**
- Verify host's IP address
- Check if port 7777 is blocked
- Try disabling firewall temporarily

**Game desync issues?**
- All players should use the same game version
- Check network stability
- Host should have stable connection

**Windows-specific issues?**
- Run as Administrator if needed
- Check Windows Defender firewall
- Ensure MSYS2/MinGW is properly installed

### Advanced Network Settings

Edit `bin/init.txt` to configure:
```
# Network settings
network_enabled=1
default_port=7777
max_players=8
discovery_interval=1000
connection_timeout=5000
```

### Performance Tips

- **Host requirements**: Stable connection, sufficient bandwidth
- **Recommended**: 100 Mbps+ for 8 players
- **Latency**: < 50ms for optimal experience
- **Packet loss**: < 1% for stable gameplay

## Thanks to:

- [Nils Dagsson Moskopp](https://github.com/erlehmann) for very useful
  feedback on the alpha and beta versions.
- zugz (from the tigsource forum) for very useful feedback on the beta.
- Serge Zaitsev's [cucu](https://zserge.com/posts/cucu-part1/) for a very clear explanation of how to write a simple C compiler.
- Batuhan Bozkurt's [otomata](http://www.earslap.com/page/otomata.html) for the basis of the cellular automata-based procedural music generation.
