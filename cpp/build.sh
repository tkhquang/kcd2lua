#!/bin/bash
set -e

# Ensure git submodules are initialized and updated
git submodule init
git submodule update --recursive

# Checkout Lua to specific version
cd lua
git checkout v5.1.1
cd ..

# Clean build directory
rm -rf build

# Create build directory
mkdir -p build

# Configure and build with verbose output
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=ON
cmake --build build --config Release --verbose
