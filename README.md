# Mini Autonomy System

A real-time embedded perception system for autonomous applications, featuring object detection, tracking, and sensor fusion.

## Features

- **Real-time Video Pipeline**: GStreamer-based video capture with hardware acceleration support
- **Object Detection**: YOLO-compatible detector with TensorRT optimization (placeholder)
- **Multi-Object Tracking**: SORT-like tracker with Kalman filtering
- **Sensor Fusion**: Visual-Inertial Odometry (VIO) with Extended Kalman Filter
- **Performance Metrics**: FPS and latency monitoring
- **Thread-safe Architecture**: Multi-threaded pipeline with lock-free queues

## Architecture

```
Camera/Video → Decoder → Detector → Tracker → Fusion → Visualization
     ↓              ↓         ↓        ↓        ↓         ↓
   GStreamer    OpenCV   YOLO/TensorRT  SORT    EKF     OpenCV GUI
```

## Dependencies

- **Required**:
  - C++23 compatible compiler
  - OpenCV 4.x (core, highgui, imgproc, imgcodecs, videoio)
  - CMake 3.28+
  - pthreads

- **Optional**:
  - GStreamer 1.x (for RTSP/network streams)
  - TensorRT (for GPU acceleration)
  - pkg-config

## Building

### Quick Start
```bash
# Install dependencies (Ubuntu/Debian)
./build_and_test.sh deps

# Build the project
./build_and_test.sh build

# Or use the simple build script
./build.sh
```

### Advanced Build Options
```bash
# Build with tests enabled
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
make -j$(nproc)

# Build with specific compiler
CXX=clang++ ./build_and_test.sh build

# Debug build
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

## Running the Demo

### Basic Demo
```bash
# With demo video (place demo_video.mp4 in current directory)
./run_demo.sh

# With specific video file
./run_demo.sh path/to/your/video.mp4

# With camera (use camera index 0)
./run_demo.sh 0

# Direct execution
./build/perception_demo demo_video.mp4
```

### Testing and Benchmarking
```bash
# Run all tests (requires -DBUILD_TESTING=ON)
./build_and_test.sh test

# Run performance benchmarks
./build_and_test.sh benchmark

# Run everything (build, test, benchmark)
./build_and_test.sh all

# Individual test execution
./build/test_core
./build/test_vision
./build/perception_benchmark
```

### Configuration
The system uses a configuration file `config.txt` for parameter tuning:

```bash
# Edit configuration
nano config.txt

# Or create custom config
cp config.txt my_config.txt
./build/perception_demo --config my_config.txt video.mp4
```

## Demo Features

The demo shows:
- **Detection Window**: Real-time object detection with bounding boxes and confidence scores
- **Trajectory Window**: Object tracks with trajectory visualization
- **Performance Overlay**: FPS and latency metrics
- **Console Output**: Pose estimation and active track count

## Project Structure

```
Mini_Autonomy_System/
├── CMakeLists.txt              # Main build configuration
├── build.sh                    # Simple build script
├── build_and_test.sh           # Comprehensive build and test script
├── run_demo.sh                 # Demo runner script
├── config.txt                  # Configuration file
├── docker-compose.yml          # Docker deployment
├── Dockerfile                  # Container build
├── include/                    # Header files
│   ├── core/                   # Core infrastructure
│   │   ├── pipeline.hpp
│   │   ├── metrics.hpp
│   │   ├── thread_safe_queue.hpp
│   │   ├── config.hpp
│   │   ├── logger.hpp
│   │   └── benchmark.hpp
│   ├── io/                     # Input/Output modules
│   │   ├── gstreamer_capture.hpp
│   │   └── imu_simulator.hpp
│   └── vision/                 # Computer vision modules
│       ├── detector.hpp
│       ├── tracker.hpp
│       └── fusion.hpp
├── src/                        # Implementation files
│   ├── core/
│   ├── io/
│   ├── vision/
│   └── main.cpp                # Main demo application
├── tests/                      # Unit tests
│   ├── test_core.cpp
│   └── test_vision.cpp
├── apps/                       # Additional applications
│   └── benchmark.cpp           # Performance benchmarking
└── README.md                   # This file
```

## Implementation Details

### Core Pipeline
- **ThreadSafeQueue**: Lock-free queue for inter-thread communication
- **ThreadedPipeline**: Multi-stage processing pipeline
- **Metrics**: Real-time performance monitoring

### Vision Modules
- **Detector**: Placeholder for YOLO/TensorRT object detection
- **Tracker**: SORT-like multi-object tracking with Kalman filtering
- **Fusion**: Visual-inertial sensor fusion with EKF

### IO Modules
- **GStreamerCapture**: Hardware-accelerated video capture
- **IMUSimulator**: Simulated IMU data with realistic noise

## Performance Characteristics

- **Target FPS**: 30 FPS
- **Latency**: < 50ms end-to-end
- **Memory**: Optimized for embedded systems
- **CPU**: Multi-core utilization

## Extending the System

### Adding Real Object Detection
Replace the placeholder in `src/vision/detector.cpp`:
```cpp
#ifdef HAVE_TENSORRT
// Load your TensorRT engine
bool Detector::load_tensorrt_model(const std::string& engine_path) {
    // Your TensorRT implementation
}
#endif
```

### Adding Real IMU
Replace the simulator in `src/io/imu_simulator.cpp` with actual IMU integration.

### ROS2 Integration
Add ROS2 publishers in the fusion stage:
```cpp
#include <rclcpp/rclcpp.hpp>
// Publish detections, tracks, and pose
```

## Troubleshooting

### Build Issues
- Ensure OpenCV is installed with development headers
- Check C++23 compiler support
- Verify pkg-config for optional dependencies

### Runtime Issues
- Check video file format compatibility
- Verify camera permissions
- Monitor system resources

## License

This project is provided as a demonstration framework for embedded perception systems.

## Contributing

Feel free to submit issues and enhancement requests!
