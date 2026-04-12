#!/bin/bash
# Script to create video from rosbag_viz_demo_headless with subtitles

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
FRAMES_DIR="$PROJECT_ROOT/viz_frames"
VIDEO_OUTPUT="$PROJECT_ROOT/rosbag_viz_demo.mp4"
SUBTITLE_FILE="$PROJECT_ROOT/scripts/subtitles.srt"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Creating ROSBAG Visualization Video ===${NC}"

# Check if ffmpeg is available
if ! command -v ffmpeg &> /dev/null; then
    echo -e "${RED}Error: ffmpeg is not installed${NC}"
    exit 1
fi

# Check if rosbag_viz_demo_headless is built
if [ ! -f "$PROJECT_ROOT/build/rosbag_viz_demo_headless" ]; then
    echo -e "${YELLOW}Building rosbag_viz_demo_headless...${NC}"
    cd "$PROJECT_ROOT/build"
    source /opt/ros/jazzy/setup.bash
    ninja rosbag_viz_demo_headless
    cd "$PROJECT_ROOT"
fi

# Clean previous frames
if [ -d "$FRAMES_DIR" ]; then
    echo -e "${YELLOW}Cleaning previous frames...${NC}"
    rm -rf "$FRAMES_DIR"
fi
mkdir -p "$FRAMES_DIR"

# Run headless demo to save frames
echo -e "${GREEN}Running headless demo to save frames...${NC}"
cd "$PROJECT_ROOT"
source /opt/ros/jazzy/setup.bash
./build/rosbag_viz_demo_headless demo/data/dataset_ros2 "$FRAMES_DIR"

# Count frames
FRAME_COUNT=$(ls -1 "$FRAMES_DIR"/*.png 2>/dev/null | wc -l)
echo -e "${GREEN}Saved $FRAME_COUNT frames${NC}"

if [ "$FRAME_COUNT" -eq 0 ]; then
    echo -e "${RED}Error: No frames were saved${NC}"
    exit 1
fi

# Create subtitle file
echo -e "${GREEN}Creating subtitle file...${NC}"
cat > "$SUBTITLE_FILE" << 'EOF'
1
00:00:00,000 --> 00:00:03,000
ROSBAG Visualization Demo

2
00:00:03,000 --> 00:00:06,000
Reading data from demo/data/dataset_ros2

3
00:00:06,000 --> 00:00:09,000
Opening ROS 2 bag file
Deserializing sensor_msgs::msg::Image

4
00:00:09,000 --> 00:00:12,000
Image format: mono16 (16-bit grayscale)
Converted to 8-bit BGR for display

5
00:00:12,000 --> 00:00:15,000
IMU Data Overlay (green text)
Shows accelerometer and gyroscope measurements

6
00:00:15,000 --> 00:00:18,000
IMU samples synchronized within ±100ms
of image timestamp for VIO algorithms

7
00:00:18,000 --> 00:00:21,000
Real data from ROS 2 bag:
872 image messages, 21741 IMU messages

8
00:00:21,000 --> 00:00:24,000
Frame info: number, timestamp, accel (m/s²), gyro (rad/s)

9
00:00:24,000 --> 00:00:27,000
Full ROS 2 message deserialization
using rclcpp::Serialization API

10
00:00:27,000 --> 00:00:30,000
End of demo - up to 100 frames processed
EOF

# Calculate video duration (assuming 30fps)
DURATION=$((FRAME_COUNT / 3))
echo -e "${GREEN}Video duration: ${DURATION}s${NC}"

# Create video from frames
echo -e "${GREEN}Creating video from frames...${NC}"
ffmpeg -framerate 30 -i "$FRAMES_DIR/frame_%04d.png" \
       -c:v libx264 -preset medium -crf 23 \
       -pix_fmt yuv420p \
       -t "$DURATION" \
       "$VIDEO_OUTPUT" \
       -y

# Add subtitles to the video
echo -e "${GREEN}Adding subtitles...${NC}"
ffmpeg -i "$VIDEO_OUTPUT" \
       -vf "subtitles='$SUBTITLE_FILE':force_style='Fontsize=28,PrimaryColour=&H00ff00&,BorderStyle=1,OutlineColour=&H000000&,Outline=2'" \
       -c:a copy \
       -c:v libx264 -preset medium -crf 23 \
       "${VIDEO_OUTPUT%.mp4}_with_subs.mp4" \
       -y

# Cleanup
rm -rf "$FRAMES_DIR"
rm -f "$SUBTITLE_FILE"
rm -f "$VIDEO_OUTPUT"

echo -e "${GREEN}Video created successfully!${NC}"
echo -e "${GREEN}Output: ${VIDEO_OUTPUT%.mp4}_with_subs.mp4${NC}"
