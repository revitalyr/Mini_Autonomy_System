#!/bin/bash

# Mini Autonomy System Demo Runner
# This script runs the perception demo with a sample video

set -e

echo "=== Mini Autonomy System Demo Runner ==="

# Change to the script's directory to ensure relative paths work
cd "$(dirname "$0")"

# Check if executable exists
EXECUTABLE="./build/perception_demo"
if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: Executable not found at $EXECUTABLE"
    echo "Please run ./build.sh first to build the project."
    exit 1
fi

# Find demo video
VIDEO_FILE="demo_video.mp4"
if [ $# -eq 1 ]; then
    VIDEO_FILE="$1"
elif [ ! -f "$VIDEO_FILE" ]; then
    echo "Warning: demo_video.mp4 not found in current directory."
    echo "You can:"
    echo "  1. Place a video file named 'demo_video.mp4' in this directory"
    echo "  2. Specify a video file as argument: ./run_demo.sh path/to/video.mp4"
    echo "  3. Use a camera: ./run_demo.sh 0"
    echo ""
    echo "Using sample online video (Big Buck Bunny)..."
    VIDEO_FILE="https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4"
fi

# Check if the input is a URL and download it if necessary
if [[ "$VIDEO_FILE" == http* ]]; then
    echo "URL detected. Downloading video to local file..."
    if command -v wget &> /dev/null; then
        wget -O downloaded_video.mp4 "$VIDEO_FILE"
        VIDEO_FILE="downloaded_video.mp4"
    elif command -v curl &> /dev/null; then
        curl -L -o downloaded_video.mp4 "$VIDEO_FILE"
        VIDEO_FILE="downloaded_video.mp4"
    fi
fi

echo "Starting demo with video source: $VIDEO_FILE"
echo "Press ESC in the detection window to exit"
echo ""

# Run the demo
$EXECUTABLE "$VIDEO_FILE"

echo ""
echo "Demo completed."
