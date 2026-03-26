#!/bin/bash

# Docker Demo Script for Mini Autonomy System
# This script builds and runs the perception system in Docker containers

set -e

echo "🚀 Mini Autonomy System Docker Demo"
echo "===================================="

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    echo "❌ Docker is not running. Please start Docker first."
    exit 1
fi

# Check if we're in the docker directory
if [[ ! -f "docker-compose.yml" ]]; then
    echo "❌ Please run this script from the docker/ directory"
    exit 1
fi

# Если передан URL, скачиваем его, иначе используем аргументы как есть
if [[ $1 == http* ]]; then
    export DEMO_ARGS="demo_video.mp4"
    # В фоновом режиме или перед запуском можно добавить wget, 
    # но для тестов лучше использовать локальные файлы
fi
export DEMO_ARGS=${1:-"demo_video.mp4"}

echo "📦 Building the perception system..."
docker-compose --profile build build

echo "🔧 Building the project inside container..."
docker-compose --profile build up --build

echo "🎥 Starting perception demo..."
echo "📺 Make sure X11 forwarding is enabled if running on Linux"
echo "🎯 The demo will show real-time object detection and tracking"
echo ""

# Run the demo
docker-compose --profile demo up

echo ""
echo "✅ Demo completed!"
echo ""
echo "📊 Performance metrics:"
echo "   - Target: 30 FPS real-time processing"
echo "   - Latency: <50ms end-to-end pipeline"
echo "   - Features: Detection, Tracking, Sensor Fusion"
echo ""
echo "🔧 To run individual services:"
echo "   Build only: docker-compose --profile build up"
echo "   Demo only:  docker-compose --profile demo up"
