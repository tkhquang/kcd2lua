#!/bin/bash
set -e

# Clean build directory
rm -rf build

# Create build directory
mkdir -p build

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
