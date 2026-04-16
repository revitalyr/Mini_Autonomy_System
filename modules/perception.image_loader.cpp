/**
 * @file perception.image_loader.cpp
 * @brief Implementation of image loading functions
 */

module;

#include <opencv2/core/version.hpp>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <ranges>
#include <algorithm>
#include <cctype>
#include <coroutine>
#include <perception/constants.h>

module perception.image_loader;

import perception.types;

namespace perception::io {

using namespace perception::constants;

/**
 * @brief Load a single image file
 * @param path Path to image file
 * @return Optional containing ImageData or nullopt if loading fails
 */
auto loadImageFile(const std::filesystem::path& path) -> Optional<ImageData> {
    cv::Mat img = cv::imread(path.string(), cv::IMREAD_COLOR);
    if (img.empty()) return std::nullopt;
    
    return ImageData::fromRaw(
        reinterpret_cast<const uint8_t*>(img.data), img.cols, img.rows, PixelFormat::Bgr, true
    );
}

/**
 * @brief Load all images from a directory as a generator
 * @param directory Path to directory containing images
 * @return Generator yielding ImageData objects
 * @details Supports jpg, jpeg, png, and bmp formats
 */
auto loadImagesFromDirectory(const std::filesystem::path& directory) -> Generator<ImageData> {
    if (!std::filesystem::exists(directory)) co_return;

    Vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            std::ranges::transform(ext, ext.begin(), ::tolower);
            if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp") {
                files.push_back(entry.path());
            }
        }
    }

    std::ranges::sort(files);

    for (const auto& path : files) {
        if (auto img = loadImageFile(path)) {
            co_yield std::move(*img);
        }
    }
}

/**
 * @brief Generate synthetic test frames
 * @param count Number of frames to generate
 * @return Generator yielding synthetic ImageData objects
 * @details Creates grayscale gradient patterns for testing
 */
auto generateTestFrames(Count count) -> Generator<ImageData> {
    for (Count i = 0; i < count; ++i) {
        int width = 640;
        int height = 480;
        cv::Mat test_mat(height, width, CV_8UC3);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                uint8_t value = static_cast<uint8_t>((x + y + i * 10) % 256);
                test_mat.at<cv::Vec3b>(y, x) = cv::Vec3b(value, value, value);
            }
        }

        co_yield ImageData::fromRaw(test_mat.data, width, height, PixelFormat::Bgr, true);
    }
}

/**
 * @brief Find the demo data directory
 * @return Path to demo data directory or empty path if not found
 * @details Searches common relative paths for demo data
 */
auto findDemoDataDirectory() -> std::filesystem::path {
    const std::vector<std::filesystem::path> paths = {
        "demo/data", "../demo/data", "../../demo/data"
    };
    for (const auto& p : paths) {
        if (std::filesystem::exists(p) && std::filesystem::is_directory(p)) return p;
    }
    return {};
}

}