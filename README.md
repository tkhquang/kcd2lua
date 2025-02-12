# KCD2 Lua Development Tools

This project provides tools for Lua development in Kingdom Come: Deliverance 2, consisting of:
- A VS Code extension for sending Lua code to the game
- A DLL mod (`.asi`) that enables convenient runtime Lua code execution in the game

It is loaded into the game with the [Ultimate-ASI-Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) and communicates with VSCode via a local TCP socket to maintain compatability with the game running on Linux under Wine.

## Building

### Prerequisites

You'll need the following packages installed. On Arch/Arched based distros:

```bash
# Basic build tools
sudo pacman -S base-devel git cmake

# MinGW-w64 cross-compiler toolchain
sudo pacman -S mingw-w64-gcc

# Node.js and npm (for VS Code extension)
sudo pacman -S nodejs npm
```

For other Linux distributions, install equivalent packages using your package manager.

On Windows you will need CMake and a compatible Visual Studio version.
```ps1
winget install cmake
winget install --id=Microsoft.VisualStudio.2022.Community  -e
```

### Building the DLL Mod - Linux

Clone the repository with submodules:
```bash
git clone --recursive https://github.com/yobson1/kcd2lua.git
cd kcd2lua
```

Navigate to the cpp directory and run the build script:
```bash
cd cpp
chmod +x build.sh
./build.sh
```

### Building the DLL Mod - Windows

Clone the repository with submodules:
```ps1
git clone --recursive https://github.com/yobson1/kcd2lua.git
cd kcd2lua
```

Generate a Visual Studio solution:
```ps1
cd cpp
cmake -B build -G "Visual Studio 17 2022" .
```
Open the .sln in Visual Studio and build

### Building the VS Code Extension

```bash
# Navigate to the vscode_extension directory
cd vscode_extension

# Install dependencies
npm install

# Build the extension
npm run compile
```

## Installation

### KCD2 Mod

- Install [Ultimate-ASI-Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) to the same directory as your KingdomCome.exe `KingdomComeDeliverance2/Bin/Win64MasterMasterSteamPGO/`
- Copy the kcd2lua.asi file to the same directory

### VS Code Extension

Install from the VS Code Marketplace: TODO: Put link here

## Usage

1. Open a Lua file in VS Code
2. Use the command palette (Ctrl+Shift+P) and search for "KCD2: Run Lua Code"
3. The code will be sent to the game and executed
4. Output will appear in the output panel in VS Code

## Debugging

- The DLL mod creates a `kcd2lua.log` file in the game directory
- If launched with `-console`, debug output will also appear in a separate console window
- The VS Code extension outputs connection status and Lua execution results in its output panel

## Acknowledgements
[Oren/ecaii](https://github.com/ecaii) - [Original code](https://github.com/ecaii/kcd2-lua-extension)
