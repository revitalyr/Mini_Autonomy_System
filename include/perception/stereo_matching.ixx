/**
 * @file perception.stereo_matching.ixx
 * @brief Stereo vision and depth estimation
 */

module;

#include <memory>

export module perception.stereo_matching;

import perception.types;
import perception.geom;
import perception.calibration;

export namespace perception::stereo {

    /**
     * @brief Opaque implementation for StereoMatching
     */
    struct StereoMatchingImpl;

    /**
     * @brief Computes disparity and depth maps from rectified stereo image pairs.
     */
    class StereoMatching {
    public:
        explicit StereoMatching(const StereoMatchingConfig& config = StereoMatchingConfig());
        ~StereoMatching();
        StereoMatching(StereoMatching&&) noexcept;
        StereoMatching& operator=(StereoMatching&&) noexcept;
        StereoMatching(const StereoMatching&) = delete;
        StereoMatching& operator=(const StereoMatching&) = delete;

        /**
         * @brief Computes the disparity map between two rectified images
         */
        [[nodiscard]] auto computeDisparity(const ImageData& left, const ImageData& right) const -> ImageData;

        /**
         * @brief Computes the depth map from a disparity map and Q matrix
         */
        [[nodiscard]] auto computeDepth(const ImageData& disparity_map, const calibration::CameraMatrix& q_matrix) const -> ImageData;

    private:
        UniquePtr<StereoMatchingImpl> m_impl;
    };

} // namespace perception::stereo