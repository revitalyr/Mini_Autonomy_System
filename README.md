# Mini Autonomy System

C++20 modules-based perception system for autonomous applications. Implements object detection, image processing, and asynchronous operations with OpenCV integration.

## System Overview

The system provides a modular perception pipeline with the following components:

- **C++20 Modules**: Module-based architecture with explicit interface boundaries
- **OpenCV Integration**: OpenCV 4.x for computer vision operations
- **Thread-safe Architecture**: Multi-threaded pipeline with thread-safe queues
- **Performance Metrics**: FPS and latency monitoring
- **Asynchronous Operations**: Coroutine-based async operations
- **Error Handling**: Result type for error propagation
- **Object Detection**: Motion-based detection with background subtraction
- **Image Loading**: Batch image loading with generator pattern
- **PIMPL Pattern**: OpenCV isolated in implementation, standard types in interfaces

## Architecture

```
Camera Input → Image Loader → Detector → Tracker → Metrics → Output
     ↓              ↓           ↓         ↓        ↓       ↓
   ImageData       Async     Motion   Kalman   Stats  Console
```

## Dependencies

- C++20 compatible compiler (MSVC 19.30+, GCC 11+, Clang 13+)
- OpenCV 4.x (core, imgproc, imgcodecs, calib3d, video)
- CMake 3.28+
- vcpkg package manager
- Catch2 v3 (for testing)

## Build Instructions

### Prerequisites
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat  # Windows
# or
./bootstrap-vcpkg.sh  # Linux/macOS

.\vcpkg install opencv4[contrib]:x64-windows  # Windows
# or
./vcpkg install opencv4[contrib]:x64-linux     # Linux
```

### Build
```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

### Runtime Dependencies (Windows)
Copy OpenCV DLLs to build directory:
- opencv_core4.dll
- opencv_imgcodecs4.dll
- opencv_imgproc4.dll
- opencv_highgui4.dll
- opencv_calib3d4.dll
- opencv_video4.dll
- zlibd1.dll

## Execution

```bash
cd build
./perception_demo.exe  # Windows
./perception_demo      # Linux/macOS
```

## Project Structure

```
Mini_Autonomy_System/
├── CMakeLists.txt              # C++20 modules build configuration
├── include/                    # Module interface files (.ixx)
│   └── perception/
│       ├── types.ixx               # Common data types
│       ├── concepts.ixx            # Core concepts
│       ├── result.ixx              # Error handling
│       ├── async.ixx               # Async operations
│       ├── queue.ixx               # Thread-safe queue
│       ├── metrics.ixx             # Performance metrics
│       ├── detector.ixx            # Object detector interface
│       ├── tracking.ixx           # Object tracking interface
│       ├── calibration.ixx        # Camera calibration interface
│       ├── stereo_matching.ixx    # Stereo vision interface
│       ├── image_loader.ixx        # Image loader interface
│       ├── pipeline.ixx            # Pipeline orchestration
│       └── constants.h             # System constants
├── modules/                    # Implementation files (.cpp)
│   ├── perception.types.cpp       # Types implementation
│   ├── perception.detector.cpp    # Detector implementation
│   ├── perception.tracking.cpp    # Tracker implementation
│   ├── perception.calibration.cpp # Calibration implementation
│   ├── perception.stereo_matching.cpp # Stereo matching implementation
│   ├── perception.image_loader.cpp # Image loader implementation
│   ├── perception.pipeline.cpp    # Pipeline implementation
│   └── perception.viz.cpp         # Visualization implementation
├── src/
│   └── main.cpp                # Demo application
├── tests/                      # Catch2 v3 tests
└── build/                      # Build output directory
```

## Module Reference

### perception.types
Core data structures:
- `ImageData` - Image container with pixel buffer management
- `Rect` - Bounding box representation
- `PixelFormat` - Supported pixel formats (BGR, RGB, Gray, YUV)
- Type aliases for STL containers

### perception.geom
Geometric primitives:
- `Rect` - 2D rectangle for bounding boxes
- `Point3D` - 3D point for spatial coordinates
- `Detection` - 2D detection result
- `Detection3D` - 3D detection result

### perception.detector
Object detection algorithms:
- Motion-based detection using background subtraction
- HOG-based pedestrian detection
- DNN-based detection (YOLO support)
- PIMPL pattern isolating OpenCV dependencies

### perception.tracking
Multi-object tracking:
- Kalman filter-based state estimation
- IoU-based data association
- Track lifecycle management
- Constant velocity motion model

### perception.calibration
Camera calibration:
- Intrinsic and extrinsic parameters
- Lens distortion correction
- Stereo rectification
- YAML serialization

### perception.stereo_matching
Stereo vision algorithms:
- Block matching (BM)
- Semi-global block matching (SGBM)
- Disparity map computation
- 3D point cloud generation

### perception.pipeline
Asynchronous processing pipeline:
- Thread-safe image queue
- Thread pool for parallel processing
- Performance metrics integration
- Graceful shutdown

### perception.metrics
Performance monitoring:
- FPS calculation
- Latency tracking
- Frame counting
- System uptime

### perception.viz
Visualization functions:
- Bounding box rendering
- Track visualization
- Window management

## API Usage

### Object Detection
```cpp
import perception.detector;
import perception.types;

auto detector = std::make_unique<perception::Detector>();
ImageData frame = /* load image */;
auto detections = detector->detect(frame);
```

### Object Tracking
```cpp
import perception.tracking;

auto tracker = std::make_unique<perception::Tracker>(0.3f, 10);
auto tracks = tracker->update(detections);
```

### Camera Calibration
```cpp
import perception.calibration;

auto calib = CameraCalibration::fromYamlFile("calibration.yaml");
auto undistorted = calib.undistort(image);
```

### Stereo Processing
```cpp
import perception.stereo_matching;

auto stereo = std::make_unique<StereoMatching>(config);
auto disparity = stereo->computeDisparity(left, right);
auto depth = stereo->computeDepth(disparity, q_matrix);
```

## Performance Characteristics

- Target FPS: 30+ (motion detection)
- Latency: < 20ms per frame (single-threaded)
- Memory: ~50MB baseline, scales with image resolution
- Thread pool: Configurable (default: 4 threads)

## Known Limitations

- Motion detection requires static camera
- DNN support requires ONNX model files
- Stereo matching requires calibrated camera pair
- Windows requires manual DLL deployment

## License

This project is provided as a demonstration framework for C++20 modules and perception systems.
