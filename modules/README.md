# Mini Autonomy System

A modern C++20 modules-based perception system for autonomous applications, featuring object detection, image processing, and async operations with full OpenCV integration.

## Features

- **C++20 Modules**: Modern module-based architecture with clean interfaces
- **OpenCV Integration**: Full integration with OpenCV 4.x for computer vision operations
- **Thread-safe Architecture**: Multi-threaded pipeline with lock-free queues
- **Performance Metrics**: Real-time FPS and latency monitoring
- **Async Operations**: Coroutine-based async operations
- **Error Handling**: std::expected-based error handling
- **Object Detection**: Motion-based object detection with background subtraction
- **Async Image Loading**: Batch image loading with thread pool
- **DNN Integration**: Support for DNN models (e.g., YOLOv8) via OpenCV's dnn module
- **PIMPL Pattern**: OpenCV isolated in implementation, std types in interfaces

## Architecture

```
Camera Input → Image Loader → Detector → Metrics → Output
     ↓              ↓           ↓        ↓       ↓
   ImageData       Async     Motion   Stats  Console
```

## Dependencies

- **Required**:
  - C++20 compatible compiler (MSVC 19.30+, GCC 11+, Clang 13+)
  - OpenCV 4.x (core, imgproc, imgcodecs)
  - CMake 3.28+
  - vcpkg package manager
  - Catch2 v3 (for testing)

## Building with C++20 Modules

### Prerequisites
```bash
# Install vcpkg (if not already installed)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat  # On Windows
# or
./bootstrap-vcpkg.sh  # On Linux/macOS

# Install OpenCV via vcpkg
.\vcpkg install opencv4[contrib]:x64-windows  # Windows
# or
./vcpkg install opencv4[contrib]:x64-linux     # Linux
```

### Build Steps
```bash
# Configure with vcpkg toolchain
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build the project
cmake --build build --config Release

# Copy required OpenCV DLLs (Windows only)
copy D:\tools\vcpkg\installed\x64-windows\bin\opencv_core4.dll build\
copy D:\tools\vcpkg\installed\x64-windows\bin\opencv_imgcodecs4.dll build\
copy D:\tools\vcpkg\installed\x64-windows\bin\opencv_imgproc4.dll build\
copy D:\tools\vcpkg\installed\x64-windows\bin\opencv_highgui4.dll build\
copy D:\tools\vcpkg\installed\x64-windows\debug\bin\zlibd1.dll build\
```

## Running the Demo

```bash
# Run the perception system demo
cd build
./perception_demo.exe  # Windows
# or
./perception_demo      # Linux/macOS
```

### Expected Output
```
=== Mini Autonomy System Demo (C++20 Modules) ===
Program started successfully!
Creating queue...
Queue created!
Creating metrics...
Metrics created!
Creating detector...
Detector created!
Creating thread pool...
Thread pool created!
Testing thread-safe queue...
Popped value: 42
Testing performance metrics...
Testing detector...
Found 2 detections
  Detection: class=person confidence=0.8 bbox=(50,50,100,200)
  Detection: class=car confidence=0.6 bbox=(200,100,150,80)
Testing thread pool...
Thread pool result: 42

=== Performance Metrics ===
FPS: 16.129
Average Latency: 10.8667ms
Total Frames: 3
Uptime: 0.186s

Demo completed successfully!
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
│       ├── image_loader.ixx        # Image loader interface
│       └── pipeline.ixx            # Pipeline orchestration
├── modules/                    # Implementation files (.cpp)
│   ├── perception.detector.cpp     # Detector implementation
│   └── perception.image_loader.cpp  # Image loader implementation
├── src/
│   └── main.cpp                # Demo application
├── tests/                      # Catch2 v3 tests
├── build/                      # Build output directory
└── README.md                   # This file
```

## Module Documentation

### perception.types
Common data types:
- `ImageData` - Raw image data with pixel buffer
- `Rect` - Bounding box coordinates
- `Config` - Configuration settings
- `Expected<T, E>` - Error handling wrapper
- Type aliases for containers and smart pointers

### perception.concepts
Type safety concepts:
- `Detector<T>` - Object detection interfaces
- `Queue<T>` - Thread-safe queue requirements
- `MetricsProvider<T>` - Performance monitoring interfaces

### perception.result
Error handling:
- `Result<T>` - Custom result type for error handling
- `PerceptionError` - Domain-specific error codes
- `AsyncResult<T>` - Async result wrapper

### perception.async
Async operations:
- `Task<T>` - Coroutine-based async tasks
- `Generator<T>` - Lazy sequence generation
- `ThreadPool` - Thread pool for async execution

### perception.queue
Thread-safe data structures:
- `ThreadSafeQueue<T>` - Lock-free queue implementation
- Timeout-based operations
- Shutdown-safe design

### perception.metrics
Performance monitoring:
- `PerformanceMetrics` - FPS and latency tracking
- `MetricsSnapshot` - Current performance state

### perception.detector
Motion-based object detector:
- Background subtraction for motion detection
- Contour-based object detection
- Classification into person, car, bicycle, object
- PIMPL pattern with OpenCV isolated in implementation

### perception.image_loader
Async image loading:
- Generator-based lazy loading
- Batch image loading from directories
- Support for JPEG, PNG, BMP, TIFF, WebP
- Thread pool for parallel loading

## API Examples

### Using the Thread-safe Queue
```cpp
import perception.queue;

auto queue = std::make_unique<perception::ThreadSafeQueue<int>>();
queue->push(42);
auto result = queue->try_pop();
if (result) {
    std::cout << "Got: " << *result << std::endl;
}
```

### Using Performance Metrics
```cpp
import perception.metrics;

auto metrics = std::make_unique<perception::PerformanceMetrics>();
metrics->record_frame_latency(10.5);
auto snapshot = metrics->get_snapshot();
std::cout << "FPS: " << snapshot.fps << std::endl;
```

### Using Object Detection
```cpp
import perception.detector;
import perception.types;

auto detector = std::make_unique<perception::Detector>();
ImageData frame = /* load image */;
auto detections = detector->detect(frame);
for (const auto& det : detections) {
    std::cout << "Found: " << det.class_name << std::endl;
}
```

### Using Async Operations
```cpp
import perception.async;

ThreadPool pool;
auto future = pool.submit([]() -> int {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 42;
});
std::cout << "Result: " << future.get() << std::endl;
```

## Implementation Details

### C++20 Modules Benefits
- **Faster Compilation**: Module-based compilation reduces build times
- **Clean Interfaces**: Explicit module boundaries
- **No Header Guards**: Modules eliminate include guard issues
- **Better Encapsulation**: Private module implementation details

### Error Handling
- Custom `Result<T>` type for C++20 compatibility
- Type-safe error codes with `PerceptionError`
- Seamless integration with `std::error_code`

### Performance Characteristics
- **Target FPS**: 30+ FPS achievable
- **Latency**: < 20ms for simple operations
- **Memory**: Efficient module-based linking
- **CPU**: Multi-core utilization with thread pool

## Troubleshooting

### Build Issues
- **Missing OpenCV**: Ensure OpenCV is installed via vcpkg
- **Module Errors**: Check C++20 compiler support
- **Link Errors**: Verify vcpkg toolchain configuration

### Runtime Issues (Windows)
- **DLL Missing**: Copy OpenCV DLLs to build directory
- **zlibd1.dll**: Required for OpenCV image operations
- **Module Loading**: Ensure all modules are properly built

### Performance Issues
- **Debug Build**: Use Release configuration for performance
- **OpenCV Optimization**: Ensure OpenCV is built with optimizations
- **Thread Count**: Adjust thread pool size for your hardware

## Extending the System

### Adding New Modules
1. Create `.ixx` interface file in `include/perception/`
2. Export module with `export module perception.newmodule;`
3. Create `.cpp` implementation file in `modules/`
4. Add to `CMakeLists.txt` module list
5. Import and use in main application

### Custom Object Detection
Extend the detector with custom implementation:
```cpp
// In perception.detector.cpp
// The current implementation uses background subtraction
// Replace with your preferred detection algorithm:
// - YOLO via OpenCV DNN
// - YOLOv8 via OpenCV DNN (now integrated!)
// - Haar cascades
// - Custom deep learning models
```

### Additional Metrics
Extend `PerformanceMetrics` class:
```cpp
// Add new metric types
void record_memory_usage(size_t bytes);
void record_gpu_utilization(double percent);
```

## License

This project is provided as a demonstration framework for modern C++20 modules and perception systems.

## Contributing

Feel free to submit issues and enhancement requests! When contributing:
1. Follow C++20 modules best practices
2. Add comprehensive Doxygen documentation
3. Include unit tests for new features
4. Update this README documentation
