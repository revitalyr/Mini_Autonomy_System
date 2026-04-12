#!/bin/bash
# Script to record rosbag_viz_demo with subtitles

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
VIDEO_OUTPUT="$PROJECT_ROOT/demo_output.mp4"
SUBTITLE_FILE="$PROJECT_ROOT/scripts/subtitles.srt"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== ROSBAG Visualization Demo Recording ===${NC}"

# Check if ffmpeg is available
if ! command -v ffmpeg &> /dev/null; then
    echo -e "${RED}Error: ffmpeg is not installed${NC}"
    exit 1
fi

# Check if rosbag_viz_demo is built
if [ ! -f "$PROJECT_ROOT/build/rosbag_viz_demo" ]; then
    echo -e "${YELLOW}Building rosbag_viz_demo...${NC}"
    cd "$PROJECT_ROOT/build"
    source /opt/ros/jazzy/setup.bash
    ninja rosbag_viz_demo
    cd "$PROJECT_ROOT"
fi

# Check if Xvfb is available for virtual display
if command -v Xvfb &> /dev/null; then
    echo -e "${GREEN}Using virtual display (Xvfb)${NC}"
    VFB_DISPLAY=:99
    Xvfb "$VFB_DISPLAY" -screen 0 1280x720x24 &
    VFB_PID=$!
    export DISPLAY="$VFB_DISPLAY"
    sleep 2
else
    echo -e "${YELLOW}Xvfb not found, using current display${NC}"
    export DISPLAY="${DISPLAY:-:0}"
fi

# Create subtitle file
echo -e "${GREEN}Creating subtitle file...${NC}"
cat > "$SUBTITLE_FILE" << 'EOF'
1
00:00:00,000 --> 00:00:05,000
ROSBAG Visualization Demo
Reading data from demo/data/dataset_ros2

2
00:00:05,000 --> 00:00:10,000
Opening ROS 2 bag file
Deserializing sensor_msgs::msg::Image (mono16 format)

3
00:00:10,000 --> 00:00:15,000
Image size: 512x512 pixels
Converting mono16 to 8-bit grayscale then to BGR for display

4
00:00:15,000 --> 00:00:20,000
IMU Data Overlay
Shows accelerometer and gyroscope measurements synchronized with image timestamps

5
00:00:20,000 --> 00:00:25,000
Green text shows:
- Frame number and timestamp
- Accelerometer (m/s²)
- Gyroscope (rad/s)
- Number of IMU samples

6
00:00:25,000 --> 00:00:30,000
IMU samples are collected within ±100ms window of image timestamp
This ensures temporal alignment for VIO algorithms

7
00:00:30,000 --> 00:00:35,000
Playback at ~30fps (30ms delay)
Press 'q' or ESC to exit

8
00:00:35,000 --> 00:00:40,000
Real data from ROS 2 bag
872 image messages, 21741 IMU messages

9
00:00:40,000 --> 00:00:45,000
This demo demonstrates full ROS 2 message deserialization
using rclcpp::Serialization API

10
00:00:45,000 --> 00:00:50,000
End of demo
Total frames displayed: up to 100 (limited for demo)
EOF

# Record the demo
echo -e "${GREEN}Starting recording...${NC}"
echo -e "${YELLOW}Press Ctrl+C to stop recording${NC}"

cd "$PROJECT_ROOT"
source /opt/ros/jazzy/setup.bash

# Record with ffmpeg
ffmpeg -f x11grab -video_size 1280x720 -framerate 30 -i "$DISPLAY" \
       -c:v libx264 -preset ultrafast -crf 22 \
       -pix_fmt yuv420p \
       -t 30 \
       "$VIDEO_OUTPUT" \
       &

FFMPEG_PID=$!

# Wait a moment for ffmpeg to start
sleep 2

# Run the visualization demo
timeout 30s ./build/rosbag_viz_demo demo/data/dataset_ros2 || true

# Stop ffmpeg
kill $FFMPEG_PID 2>/dev/null || true
wait $FFMPEG_PID 2>/dev/null || true

# Kill virtual display if it was started
if [ -n "$VFB_PID" ]; then
    kill $VFB_PID 2>/dev/null || true
fi

# Add subtitles to the video
echo -e "${GREEN}Adding subtitles...${NC}"
ffmpeg -i "$VIDEO_OUTPUT" \
       -vf "subtitles='$SUBTITLE_FILE':force_style='Fontsize=24,PrimaryColour=&H00ff00&,BorderStyle=1'" \
       -c:a copy \
       -c:v libx264 -preset medium -crf 23 \
       "${VIDEO_OUTPUT%.mp4}_with_subs.mp4"

echo -e "${GREEN}Recording complete!${NC}"
echo -e "${GREEN}Video saved to: ${VIDEO_OUTPUT%.mp4}_with_subs.mp4${NC}"

# Cleanup
rm -f "$SUBTITLE_FILE"
