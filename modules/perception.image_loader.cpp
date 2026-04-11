module;
// GMF - OpenCV isolated here
#include <opencv2/opencv.hpp>

module perception.image_loader;

import perception.types;
import perception.async;
import perception.result;

#include <filesystem>
#include <vector>
#include <algorithm>
#include <memory>

namespace perception {

    // PIMPL implementation - knows about OpenCV
    struct AsyncImageLoader::Impl {
        ThreadPool thread_pool;

        Impl(size_t num_threads) : thread_pool(num_threads) {}
    };

    // --- Constructor/Destructor ---

    AsyncImageLoader::AsyncImageLoader(size_t num_threads)
        : impl_(std::make_unique<Impl>(num_threads)) {}

    AsyncImageLoader::~AsyncImageLoader() = default;

    // --- Load Images ---

    Generator<PerceptionResult<ImageData>> AsyncImageLoader::load_images(const std::filesystem::path& directory) {
        std::vector<std::filesystem::path> image_paths;

        // Collect image paths first
        try {
            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    const auto& path = entry.path();
                    const auto ext = path.extension().string();

                    static const std::vector<std::string> extensions = {".jpg", ".png", ".bmp"};
                    if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
                        image_paths.push_back(path);
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error&) {
            co_return;
        }

        // Load images asynchronously
        for (const auto& path : image_paths) {
            auto future = impl_->thread_pool.submit([path]() -> PerceptionResult<ImageData> {
                try {
                    // Load image using OpenCV
                    cv::Mat mat = cv::imread(path.string(), cv::IMREAD_COLOR);
                    if (mat.empty()) {
                        return error<ImageData>(PerceptionError::InvalidInput);
                    }

                    // Convert cv::Mat to ImageData (std types only)
                    ImageData img_data(mat.cols, mat.rows, mat.channels());
                    const auto* data = mat.data;
                    std::copy(data, data + mat.total() * mat.elemSize(), img_data.data.begin());

                    return success<ImageData>(std::move(img_data));
                } catch (const std::exception&) {
                    return error<ImageData>(PerceptionError::InvalidInput);
                }
            });

            co_yield future.get();
        }
    }

    // --- Load Single Image ---

    std::future<PerceptionResult<ImageData>> AsyncImageLoader::load_image(const std::filesystem::path& path) {
        return impl_->thread_pool.submit([path]() -> PerceptionResult<ImageData> {
            try {
                // Load image using OpenCV
                cv::Mat mat = cv::imread(path.string(), cv::IMREAD_COLOR);
                if (mat.empty()) {
                    return error<ImageData>(PerceptionError::InvalidInput);
                }

                // Convert cv::Mat to ImageData (std types only)
                ImageData img_data(mat.cols, mat.rows, mat.channels());
                const auto* data = mat.data;
                std::copy(data, data + mat.total() * mat.elemSize(), img_data.data.begin());

                return success<ImageData>(std::move(img_data));
            } catch (const std::exception&) {
                return error<ImageData>(PerceptionError::InvalidInput);
            }
        });
    }

} // namespace perception
