# Mini Autonomy System

A modern C++20 modules-based perception system for autonomous applications, featuring object detection, tracking, and sensor fusion with full OpenCV integration.

## Features

- **C++20 Modules**: Modern module-based architecture with clean interfaces
- **OpenCV Integration**: Full integration with OpenCV 4.x for computer vision operations
- **Thread-safe Architecture**: Multi-threaded pipeline with lock-free queues
- **Performance Metrics**: Real-time FPS and latency monitoring
- **Async Operations**: Coroutine-based async operations with C++20
- **Error Handling**: Custom Result<T> type for C++20 compatibility
- **Object Detection**: Mock detector with OpenCV cv::Mat and cv::Rect support
- **Thread Pool**: Efficient async task execution

## Architecture

```
Camera Input → Image Loader → Detector → Metrics → Output
     ↓              ↓           ↓        ↓       ↓
   cv::Mat        Async      OpenCV   Stats  Console
```

## Dependencies

- **Required**:
  - C++20 compatible compiler (MSVC 19.30+, GCC 11+, Clang 13+)
  - OpenCV 4.x (core, imgproc, imgcodecs, highgui)
  - CMake 3.28+
  - vcpkg package manager
  - pthreads

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
├── modules/                    # C++20 modules (.cppm files)
│   ├── perception.concepts.cppm    # Core concepts and interfaces
│   ├── perception.result.cppm      # Result type and error handling
│   ├── perception.async.cppm       # Async operations and coroutines
│   ├── perception.queue.cppm        # Thread-safe queue implementation
│   ├── perception.metrics.cppm     # Performance metrics tracking
│   ├── perception.detector.simple.cppm  # Mock object detector
│   ├── perception.image_loader.simple.cppm  # Async image loading
│   └── perception.pipeline.cppm     # Pipeline orchestration
├── src/
│   └── main.cpp                # Demo application
├── build/                      # Build output directory
└── README.md                   # This file
```

## Module Documentation

### perception.concepts
Defines C++20 concepts for type safety:
- `Detector<T>` - Object detection interfaces
- `Queue<T>` - Thread-safe queue requirements
- `MetricsProvider<T>` - Performance monitoring interfaces
- `AsyncOperation<T>` - Async operation requirements

### perception.result
Custom error handling for C++20:
- `Result<T>` - std::expected replacement for C++20
- `PerceptionError` - Domain-specific error codes
- `AsyncResult<T>` - Async result wrapper

### perception.async
Async operations and coroutines:
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
- Real-time statistics

### perception.detector.simple
Mock object detector:
- OpenCV cv::Mat integration
- cv::Rect bounding boxes
- Detection results with confidence scores

### perception.image_loader.simple
Async image loading:
- Generator-based lazy loading
- OpenCV imread integration
- Error handling with Result<T>

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

auto detector = std::make_unique<perception::MockDetector>();
cv::Mat frame = cv::imread("image.jpg");
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
1. Create `.cppm` file in `modules/` directory
2. Export module with `export module perception.newmodule;`
3. Add to `CMakeLists.txt` module list
4. Import and use in main application

### Real Object Detection
Replace `MockDetector` with actual implementation:
```cpp
// In perception.detector.real.cppm
export class RealDetector {
    std::vector<Detection> detect(const cv::Mat& frame) override {
        // Your real detection implementation
    }
};
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
