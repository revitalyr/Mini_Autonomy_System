// GMF - OpenCV isolated here
#include <opencv2/opencv.hpp>

#include <filesystem>
#include <vector>
#include <algorithm>
#include <memory>
#include <coroutine>

#include "perception/types.hpp"
#include "perception/async.hpp"
#include "perception/result.hpp"
#include "perception/image_loader.hpp"

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
        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
            co_yield error<ImageData>(PerceptionError::InvalidInput);
            co_return;
        }

        // Collect image files
        std::vector<std::filesystem::path> image_paths;
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                auto path = entry.path();
                auto ext = path.extension().string();
                // Convert to lowercase for comparison
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || ext == ".tiff" || ext == ".tif" || ext == ".webp") {
                    image_paths.push_back(path);
                }
            }
        }

        if (image_paths.empty()) {
            co_yield error<ImageData>(PerceptionError::InvalidInput);
            co_return;
        }

        // Sort paths for consistent ordering
        std::sort(image_paths.begin(), image_paths.end());

        // Load images asynchronously
        std::vector<std::future<PerceptionResult<ImageData>>> results;
        for (const auto& path : image_paths) {
            auto future = impl_->thread_pool.submit([path]() -> PerceptionResult<ImageData> {
                try {
                    // Check file exists and is readable
                    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
                        return error<ImageData>(PerceptionError::InvalidInput);
                    }

                    // Load image using OpenCV
                    cv::Mat mat = cv::imread(path.string(), cv::IMREAD_COLOR);
                    if (mat.empty()) {
                        return error<ImageData>(PerceptionError::ModelLoadFailed);
                    }

                    // Validate image dimensions
                    if (mat.cols <= 0 || mat.rows <= 0 || mat.channels() <= 0) {
                        return error<ImageData>(PerceptionError::InvalidInput);
                    }

                    // Convert cv::Mat to ImageData (std types only)
                    ImageData img_data(mat.cols, mat.rows, mat.channels());
                    const auto* data = mat.data;
                    std::copy(data, data + mat.total() * mat.elemSize(), img_data.data.begin());

                    return success<ImageData>(std::move(img_data));
                } catch (const cv::Exception&) {
                    return error<ImageData>(PerceptionError::ModelLoadFailed);
                } catch (const std::exception& e) {
                    return error<ImageData>(PerceptionError::InvalidInput);
                }
            });

            results.push_back(std::move(future));
        }

        // Wait for all images to load and yield results
        for (auto& future : results) {
            co_yield future.get();
        }
    }

    // --- Load Single Image ---

    std::future<PerceptionResult<ImageData>> AsyncImageLoader::load_image(const std::filesystem::path& path) {
        return impl_->thread_pool.submit([path]() -> PerceptionResult<ImageData> {
            try {
                if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
                    return error<ImageData>(PerceptionError::InvalidInput);
                }

                cv::Mat mat = cv::imread(path.string(), cv::IMREAD_COLOR);
                if (mat.empty()) {
                    return error<ImageData>(PerceptionError::ModelLoadFailed);
                }

                if (mat.cols <= 0 || mat.rows <= 0 || mat.channels() <= 0) {
                    return error<ImageData>(PerceptionError::InvalidInput);
                }

                ImageData img_data(mat.cols, mat.rows, mat.channels());
                const auto* data = mat.data;
                std::copy(data, data + mat.total() * mat.elemSize(), img_data.data.begin());

                return success<ImageData>(std::move(img_data));
            } catch (const cv::Exception&) {
                return error<ImageData>(PerceptionError::ModelLoadFailed);
            } catch (const std::exception&) {
                return error<ImageData>(PerceptionError::InvalidInput);
            }
        });
    }

} // namespace perception
