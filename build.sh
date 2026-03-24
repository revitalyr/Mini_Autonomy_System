#!/bin/bash

# Mini Autonomy System Build Script
# This script builds the perception system with all dependencies

set -e

echo "=== Mini Autonomy System Build Script ==="

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Please run this script from the project root."
    exit 1
fi

# Create build directory
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure build
echo "Configuring build with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the project
echo "Building project..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Build completed successfully!"
echo ""
echo "To run the demo:"
echo "  ./build/perception_demo [video_file]"
echo ""
echo "Example:"
echo "  ./build/perception_demo demo_video.mp4"
echo ""
echo "If no video file is specified, the demo will look for 'demo_video.mp4' in the current directory."
