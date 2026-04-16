/**
 * @file perception.types.cpp
 * @brief Implementation of core image data types
 */

module;

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <memory>

module perception.types;

namespace perception {

/**
 * @brief Default constructor
 */
ImageData::ImageData() : m_impl(std::make_unique<ImageImpl>()) {}

/**
 * @brief Destructor
 */
ImageData::~ImageData() = default;

/**
 * @brief Move constructor
 */
ImageData::ImageData(ImageData&&) noexcept = default;

/**
 * @brief Move assignment operator
 */
ImageData& ImageData::operator=(ImageData&&) noexcept = default;

/**
 * @brief Copy constructor
 * @param other ImageData to copy from
 * @details Performs deep copy of internal OpenCV Mat
 */
ImageData::ImageData(const ImageData& other) : m_impl(std::make_unique<ImageImpl>()) {
    if (other.m_impl && !other.m_impl->mat.empty()) {
        m_impl->mat = other.m_impl->mat.clone();
        m_width = other.m_width;
        m_height = other.m_height;
        m_channels = other.m_channels;
        m_bit_depth = other.m_bit_depth;
        m_timestamp = other.m_timestamp;
    }
}

/**
 * @brief Copy assignment operator
 * @param other ImageData to copy from
 * @return Reference to this ImageData
 */
ImageData& ImageData::operator=(const ImageData& other) {
    if (this != &other) {
        if (other.m_impl && !other.m_impl->mat.empty()) {
            m_impl->mat = other.m_impl->mat.clone();
            m_width = other.m_width;
            m_height = other.m_height;
            m_channels = other.m_channels;
            m_bit_depth = other.m_bit_depth;
            m_timestamp = other.m_timestamp;
        }
    }
    return *this;
}

/**
 * @brief Get mutable pointer to image data
 * @return Pointer to image data buffer
 */
auto ImageData::data() -> ByteBuffer {
    return m_impl ? m_impl->mat.data : nullptr;
}

/**
 * @brief Get const pointer to image data
 * @return Const pointer to image data buffer
 */
auto ImageData::data() const -> ConstByteBuffer {
    return m_impl ? m_impl->mat.data : nullptr;
}

/**
 * @brief Check if image is empty
 * @return True if image contains no data
 */
auto ImageData::empty() const -> bool {
    return !m_impl || m_impl->mat.empty();
}

/**
 * @brief Create deep copy of image data
 * @return New ImageData with copied data
 */
auto ImageData::clone() const -> ImageData {
    ImageData copy;
    if (m_impl && !m_impl->mat.empty()) {
        copy.m_impl->mat = m_impl->mat.clone();
        copy.m_width = m_width;
        copy.m_height = m_height;
        copy.m_channels = m_channels;
        copy.m_bit_depth = m_bit_depth;
        copy.m_timestamp = m_timestamp;
    }
    return copy;
}

/**
 * @brief Ensure image data is owned (not a wrapper around external memory)
 * @details Clones the internal Mat if it wraps external memory
 */
auto ImageData::ensure_owned() -> void {
    if (m_impl && m_impl->mat.u == nullptr && !m_impl->mat.empty()) {
        m_impl->mat = m_impl->mat.clone();
    }
}

/**
 * @brief Create ImageData from raw pixel buffer
 * @param data Pointer to raw pixel data
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @param format Pixel format of input data
 * @param copy_data If true, copy data; otherwise wrap external memory
 * @return ImageData created from raw buffer
 * @details Converts various pixel formats to internal BGR format
 */
auto ImageData::fromRaw(ConstByteBuffer data, int width, int height, PixelFormat format, bool copy_data) -> ImageData {
    ImageData result;
    if (!data) return result;

    result.m_width = width;
    result.m_height = height;
    result.m_bit_depth = 8;

    cv::Mat wrapper;
    bool needs_conversion = false;
    int cv_type = CV_8UC3;
    
    switch (format) {
        case PixelFormat::Bgr:
            cv_type = CV_8UC3;
            result.m_channels = 3;
            break;
        case PixelFormat::Bgr16:
            cv_type = CV_16UC3;
            result.m_channels = 3;
            result.m_bit_depth = 16;
            break;
        case PixelFormat::Bgr32F:
            cv_type = CV_32FC3;
            result.m_channels = 3;
            result.m_bit_depth = 32;
            break;
        case PixelFormat::Rgb:
            wrapper = cv::Mat(height, width, CV_8UC3, const_cast<uint8_t*>(data));
            cv::cvtColor(wrapper, result.m_impl->mat, cv::COLOR_RGB2BGR);
            result.m_channels = 3;
            needs_conversion = true;
            break;
        case PixelFormat::Gray:
            cv_type = CV_8UC1;
            result.m_channels = 1;
            break;
        case PixelFormat::Gray16:
            cv_type = CV_16UC1;
            result.m_channels = 1;
            result.m_bit_depth = 16;
            break;
        case PixelFormat::Bgra:
            cv_type = CV_8UC4;
            result.m_channels = 4;
            break;
        case PixelFormat::Rgba:
            wrapper = cv::Mat(height, width, CV_8UC4, const_cast<uint8_t*>(data));
            cv::cvtColor(wrapper, result.m_impl->mat, cv::COLOR_RGBA2BGR);
            result.m_channels = 3;
            needs_conversion = true;
            break;
        case PixelFormat::YuvI420: {
            wrapper = cv::Mat(height + height / 2, width, CV_8UC1, const_cast<uint8_t*>(data));
            cv::cvtColor(wrapper, result.m_impl->mat, cv::COLOR_YUV2BGR_I420);
            result.m_channels = 3;
            needs_conversion = true;
            break;
        }
        case PixelFormat::YuvNv12: {
            wrapper = cv::Mat(height + height / 2, width, CV_8UC1, const_cast<uint8_t*>(data));
            cv::cvtColor(wrapper, result.m_impl->mat, cv::COLOR_YUV2BGR_NV12);
            result.m_channels = 3;
            needs_conversion = true;
            break;
        }
    }

    if (!needs_conversion) {
        wrapper = cv::Mat(height, width, cv_type, const_cast<uint8_t*>(data));
        if (copy_data) {
            result.m_impl->mat = wrapper.clone();
        } else {
            result.m_impl->mat = wrapper;
        }
    }

    return result;
}
} // namespace perception
