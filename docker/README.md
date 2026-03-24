# Docker Demo - Mini Autonomy System

This folder contains Docker configuration for running the Mini Autonomy System in containers.

## Quick Start

```bash
# Run the complete demo
./run_demo.sh
```

## Manual Commands

### Build the system
```bash
docker-compose --profile build up --build
```

### Run the demo
```bash
docker-compose --profile demo up
```

## Services

### perception-build
- Builds the C++ project with all dependencies
- Installs OpenCV, GStreamer, and other required libraries
- Compiles the perception pipeline

### perception-demo
- Runs the real-time perception demo
- Displays object detection and tracking visualization
- Shows performance metrics (FPS, latency)

## Requirements

- Docker installed and running
- NVIDIA Docker runtime (for GPU acceleration)
- X11 forwarding (for GUI display on Linux)

## Environment Variables

- `DISPLAY` - X11 display for GUI output

## Volume Mounts

- `..:/workspace` - Mounts the project root
- `/tmp/.X11-unix:/tmp/.X11-unix:rw` - X11 socket for GUI

## Performance

- Target: 30 FPS real-time processing
- Latency: <50ms end-to-end pipeline
- Multi-threaded architecture with lock-free queues

## Features Demonstrated

1. **Real-time Object Detection** - YOLO/TensorRT-ready detector
2. **Multi-Object Tracking** - SORT-like tracker with Kalman filtering
3. **Sensor Fusion** - Visual-inertial fusion with Extended Kalman Filter
4. **Performance Monitoring** - FPS, latency, and throughput metrics
5. **Hardware Acceleration** - GStreamer and GPU support

## Troubleshooting

### X11 Display Issues
```bash
# Allow X11 connections
xhost +local:docker
```

### GPU Issues
Remove `runtime: nvidia` from docker-compose.yml if not using NVIDIA GPU.

### Permission Issues
Ensure Docker daemon is running and user has Docker permissions.
