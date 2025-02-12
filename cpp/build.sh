#!/bin/bash
set -e

# Ensure git submodules are initialized and updated
git submodule init
git submodule update --recursive

# Create build directory
mkdir -p build

# Configure and build with verbose output
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=ON
cmake --build build --config Release --verbose
