module;
#include <filesystem>

export module perception.image_loader;
import perception.types;
import perception.geom;
import perception.async;

export namespace perception::io {

    /**
     * @brief Load image from disk
     */
    auto loadImageFile(const std::filesystem::path& path) -> Optional<geom::ImageData>;

    /**
     * @brief Stream images from a directory using a generator
     */
    auto loadImagesFromDirectory(const std::filesystem::path& directory) -> Generator<geom::ImageData>;

    /**
     * @brief Generate synthetic frames for testing
     */
    auto generateTestFrames(Count count = 10) -> Generator<geom::ImageData>;

    /**
     * @brief Helper to find the demo data path
     */
    auto findDemoDataDirectory() -> std::filesystem::path;
}