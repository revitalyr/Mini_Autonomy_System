FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    pkg-config \
    libopencv-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    libgtk-3-dev \
    python3-dev \
    python3-numpy \
    git \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Install CMake 3.28+
RUN wget -q https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-linux-x86_64.sh && \
    chmod +x cmake-3.28.3-linux-x86_64.sh && \
    ./cmake-3.28.3-linux-x86_64.sh --prefix=/usr/local --skip-license && \
    rm cmake-3.28.3-linux-x86_64.sh

# Set working directory
WORKDIR /workspace

# Copy project files
COPY . /workspace

# Build the project
RUN sed -i 's/\r$//' build.sh && \
    chmod +x build.sh && \
    sed -i '1i #include <iomanip>' include/core/benchmark.hpp && \
    ./build.sh
