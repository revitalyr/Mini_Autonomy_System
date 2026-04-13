module;

#include <opencv2/opencv.hpp>
#include <algorithm>
#include <vector>
#include <filesystem>
#include <coroutine>

module perception.image_loader;

namespace perception::io {

auto loadImageFile(const std::filesystem::path& path) -> Optional<geom::ImageData> {
    cv::Mat img = cv::imread(path.string(), cv::IMREAD_COLOR);
    if (img.empty()) return std::nullopt;
    
    cv::Mat rgb;
    cv::cvtColor(img, rgb, cv::COLOR_BGR2RGB);

    geom::ImageData data;
    data.width = rgb.cols;
    data.height = rgb.rows;
    data.channels = rgb.channels();
    data.data.assign(rgb.data, rgb.data + (static_cast<size_t>(rgb.total()) * rgb.channels()));
    
    return data;
}

auto loadImagesFromDirectory(const std::filesystem::path& directory) -> Generator<geom::ImageData> {
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

auto generateTestFrames(Count count) -> Generator<geom::ImageData> {
    for (Count i = 0; i < count; ++i) {
        geom::ImageData frame{480, 640, 3};
        frame.data.resize(frame.width * frame.height * frame.channels);
        
        for (int y = 0; y < frame.height; ++y) {
            for (int x = 0; x < frame.width; ++x) {
                int idx = (y * frame.width + x) * frame.channels;
                uint8_t value = static_cast<uint8_t>((x + y + i * 10) % 256);
                frame.data[idx] = value;
                frame.data[idx + 1] = value;
                frame.data[idx + 2] = value;
            }
        }
        co_yield frame;
    }
}

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