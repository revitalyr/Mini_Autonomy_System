/**
 * @file perception.calibration.ixx
 * @brief Camera calibration and rectification
 */

module;

#include <vector>
#include <array>
#include <memory>
#include <string>

export module perception.calibration;

import perception.types;
import perception.result;

export namespace perception::calibration {

    /**
     * @brief Semantic aliases for calibration parameters
     */
    using CameraMatrix = std::array<double, 9>; // 3x3 row-major matrix
    using DistortionCoeffs = std::vector<double>;
    using ProjectionMatrix = std::array<double, 12>; // 3x4 row-major matrix
    using RotationMatrix = std::array<double, 9>;   // 3x3 rotation matrix
    using TranslationVector = std::array<double, 3>; // 3x1 translation vector

    /**
     * @brief Opaque implementation for CameraCalibration
     */
    struct CalibrationImpl;

    struct StereoCalibrationImpl;

    /**
     * @brief Manages camera intrinsic parameters and image rectification
     */
    class CameraCalibration {
    public:
        CameraCalibration();
        ~CameraCalibration();

        CameraCalibration(CameraCalibration&&) noexcept;
        CameraCalibration& operator=(CameraCalibration&&) noexcept;

        CameraCalibration(const CameraCalibration&) = delete;
        CameraCalibration& operator=(const CameraCalibration&) = delete;

        /**
         * @brief Sets calibration parameters. Invalidates any pre-computed rectification maps.
         */
        auto setParameters(const CameraMatrix& k, const DistortionCoeffs& d) -> void;

        /**
         * @brief Sets the free scaling alpha parameter (0.0 to 1.0)
         * @param alpha 0: zoom in (no black pixels), 1: all pixels preserved
         */
        auto setAlpha(double alpha) -> void;

        /**
         * @brief Loads calibration parameters from a YAML file.
         * @param filepath Path to the YAML file.
         * @return A CameraCalibration object on success, or an error.
         */
        [[nodiscard]] static auto fromYamlFile(const String& filepath) -> Result<CameraCalibration>;

        /**
         * @brief Saves current calibration parameters to a YAML file.
         * @param filepath Path where the YAML file will be created.
         * @return Success or error code.
         */
        [[nodiscard]] auto saveToYamlFile(const String& filepath) const -> Result<void>;

        /**
         * @brief Resets any pre-computed rectification maps.
         * This should be called if image dimensions change or if setParameters is called.
         */
        auto resetMaps() -> void;

        /**
         * @brief Undistorts an image using the current calibration
         * @param src Input image
         * @return Undistorted image
         */
        [[nodiscard]] auto undistort(const ImageData& src) const -> ImageData;

        [[nodiscard]] auto isValid() const noexcept -> bool;

    public:
        UniquePtr<CalibrationImpl> m_impl;
    };

    /**
     * @brief Manages stereo camera calibration and rectification
     */
    class StereoCameraCalibration {
    public:
        StereoCameraCalibration();
        ~StereoCameraCalibration();

        StereoCameraCalibration(StereoCameraCalibration&&) noexcept;
        StereoCameraCalibration& operator=(StereoCameraCalibration&&) noexcept;

        StereoCameraCalibration(const StereoCameraCalibration&) = delete;
        StereoCameraCalibration& operator=(const StereoCameraCalibration&) = delete;

        /**
         * @brief Sets stereo parameters including intrinsics for both cameras and extrinsics.
         */
        auto setParameters(
            const CameraCalibration& left, 
            const CameraCalibration& right,
            const RotationMatrix& r,
            const TranslationVector& t) -> void;

        /**
         * @brief Performs stereo rectification on a pair of images.
         * @return A pair of rectified images (Left, Right).
         */
        [[nodiscard]] auto rectifyStereo(const ImageData& left_src, const ImageData& right_src) const -> std::pair<ImageData, ImageData>;

        /**
         * @brief Returns the projection matrix for the left camera (P1).
         */
        [[nodiscard]] auto getProjectionMatrix1() const -> ProjectionMatrix;

        /**
         * @brief Returns the projection matrix for the right camera (P2).
         */
        [[nodiscard]] auto getProjectionMatrix2() const -> ProjectionMatrix;

        /**
         * @brief Returns the disparity-to-depth mapping matrix (Q).
         * This matrix is used to reproject 2D points with disparity into 3D space.
         */
        [[nodiscard]] auto getDisparityToDepthMatrixQ() const -> std::array<double, 16>; // Q is 4x4



        [[nodiscard]] auto isValid() const noexcept -> bool;

    private:
        UniquePtr<StereoCalibrationImpl> m_impl;
    };

} // namespace perception::calibration