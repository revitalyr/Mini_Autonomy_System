#!/bin/bash

# Comprehensive Build and Test Script for Mini Autonomy System

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Mini Autonomy System Build and Test Script ===${NC}"

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    # Check for CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake is not installed. Please install CMake 3.20 or later."
        exit 1
    fi
    
    # Check for C++ compiler
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        print_error "C++ compiler not found. Please install g++ or clang++."
        exit 1
    fi
    
    # Check for OpenCV
    if ! pkg-config --exists opencv4 2>/dev/null && ! pkg-config --exists opencv 2>/dev/null; then
        print_warning "OpenCV not found via pkg-config. Build may fail."
        print_warning "Please install OpenCV development packages:"
        print_warning "  Ubuntu/Debian: sudo apt install libopencv-dev"
        print_warning "  CentOS/RHEL: sudo yum install opencv-devel"
        print_warning "  macOS: brew install opencv"
    fi
    
    print_status "Dependencies check completed."
}

# Function to clean build
clean_build() {
    print_status "Cleaning build directory..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
    print_status "Build directory cleaned."
}

# Function to build the project
build_project() {
    print_status "Building project..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configure with CMake
    print_status "Configuring with CMake..."
    cmake .. -DCMAKE_BUILD_TYPE=Release
    
    # Build
    print_status "Compiling..."
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    print_status "Build completed successfully!"
}

# Function to run tests
run_tests() {
    print_status "Running tests..."
    
    cd "$BUILD_DIR"
    
    # Check if tests were built
    if [ -f "./test_core" ] && [ -f "./test_vision" ]; then
        print_status "Running core tests..."
        ./test_core
        
        print_status "Running vision tests..."
        ./test_vision
        
        print_status "All tests passed!"
    else
        print_warning "Tests not built. Use -DBUILD_TESTING=ON to enable tests."
    fi
}

# Function to run benchmarks
run_benchmarks() {
    print_status "Running performance benchmarks..."
    
    cd "$BUILD_DIR"
    
    if [ -f "./perception_benchmark" ]; then
        ./perception_benchmark
        print_status "Benchmarks completed. Results saved to benchmark_results.csv"
    else
        print_error "Benchmark executable not found. Build may have failed."
        exit 1
    fi
}

# Function to run demo
run_demo() {
    print_status "Running demo..."
    
    cd "$BUILD_DIR"
    
    if [ -f "./perception_demo" ]; then
        # Check for demo video
        if [ -f "$PROJECT_ROOT/demo_video.mp4" ] && [ -f "$PROJECT_ROOT/demo_video.mp4" ]; then
            print_status "Running demo with video file..."
            ./perception_demo "$PROJECT_ROOT/demo_video.mp4"
        else
            print_warning "No demo video found. You can:"
            print_warning "  1. Place a video file at demo_video.mp4"
            print_warning "  2. Specify a video file: ./run_demo.sh path/to/video.mp4"
            print_warning "  3. Use camera: ./run_demo.sh 0"
            print_status "Attempting to run with camera..."
            ./perception_demo 0
        fi
    else
        print_error "Demo executable not found. Build may have failed."
        exit 1
    fi
}

# Function to install dependencies (Ubuntu/Debian)
install_dependencies() {
    print_status "Installing dependencies..."
    
    if command -v apt-get &> /dev/null; then
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            pkg-config \
            libopencv-dev \
            libgstreamer1.0-dev \
            libgstreamer-plugins-base1.0-dev \
            gstreamer1.0-plugins-good \
            gstreamer1.0-plugins-bad \
            gstreamer1.0-plugins-ugly \
            libgtk-3-dev
            
        print_status "Dependencies installed successfully!"
    else
        print_error "Automatic dependency installation not supported on this system."
        print_error "Please install dependencies manually."
        exit 1
    fi
}

# Main script logic
case "${1:-build}" in
    "deps")
        install_dependencies
        ;;
    "clean")
        clean_build
        ;;
    "build")
        check_dependencies
        build_project
        ;;
    "test")
        check_dependencies
        build_project
        run_tests
        ;;
    "benchmark")
        check_dependencies
        build_project
        run_benchmarks
        ;;
    "demo")
        check_dependencies
        build_project
        run_demo
        ;;
    "all")
        check_dependencies
        build_project
        run_tests
        run_benchmarks
        print_status "All tasks completed successfully!"
        ;;
    "help"|"-h"|"--help")
        echo "Usage: $0 [command]"
        echo ""
        echo "Commands:"
        echo "  deps      - Install dependencies (Ubuntu/Debian only)"
        echo "  clean     - Clean build directory"
        echo "  build     - Build the project"
        echo "  test      - Build and run tests"
        echo "  benchmark - Build and run performance benchmarks"
        echo "  demo      - Build and run demo"
        echo "  all       - Build, test, and benchmark"
        echo "  help      - Show this help message"
        echo ""
        echo "Default command: build"
        ;;
    *)
        print_error "Unknown command: $1"
        print_error "Use '$0 help' for usage information."
        exit 1
        ;;
esac

echo -e "${GREEN}Script completed successfully!${NC}"
