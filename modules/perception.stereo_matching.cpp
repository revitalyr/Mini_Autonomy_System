/**
 * @file perception.stereo_matching.cpp
 * @brief Implementation of stereo vision algorithms
 */

module;

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <memory>

module perception.stereo_matching;

namespace perception::stereo {

    /**
     * @struct StereoMatchingImpl
     * @brief Private implementation details for StereoMatching
     */
    struct StereoMatchingImpl {
        StereoMatchingConfig config; ///< Stereo matching configuration
        cv::Ptr<cv::StereoBM> bm; ///< Block matching algorithm
        cv::Ptr<cv::StereoSGBM> sgbm; ///< Semi-global block matching algorithm

        /**
         * @brief Constructor
         * @param cfg Configuration parameters
         */
        explicit StereoMatchingImpl(const StereoMatchingConfig& cfg) : config(cfg) {
            if (config.use_sgbm) {
                sgbm = cv::StereoSGBM::create(
                    config.min_disparity,
                    config.num_disparities,
                    config.block_size
                );
                sgbm->setP1(config.p1 == 0 ? 8 * config.block_size * config.block_size : config.p1);
                sgbm->setP2(config.p2 == 0 ? 32 * config.block_size * config.block_size : config.p2);
                sgbm->setDisp12MaxDiff(config.disp_12_max_diff);
                sgbm->setPreFilterCap(config.pre_filter_cap);
                sgbm->setUniquenessRatio(config.uniqueness_ratio);
                sgbm->setSpeckleWindowSize(config.speckle_window_size);
                sgbm->setSpeckleRange(config.speckle_range);
                sgbm->setMode(cv::StereoSGBM::MODE_SGBM_3WAY);
            } else {
                bm = cv::StereoBM::create(config.num_disparities, config.block_size);
            }
        }
    };

    /**
     * @brief Constructor
     * @param config Stereo matching configuration
     */
    StereoMatching::StereoMatching(const StereoMatchingConfig& config)
        : m_impl(std::make_unique<StereoMatchingImpl>(config)) {}

    /**
     * @brief Destructor
     */
    StereoMatching::~StereoMatching() = default;

    /**
     * @brief Move constructor
     */
    StereoMatching::StereoMatching(StereoMatching&&) noexcept = default;

    /**
     * @brief Move assignment operator
     */
    StereoMatching& StereoMatching::operator=(StereoMatching&&) noexcept = default;

    /**
     * @brief Compute disparity map from stereo image pair
     * @param left Left camera image
     * @param right Right camera image
     * @return Disparity map as ImageData
     */
    auto StereoMatching::computeDisparity(const ImageData& left, const ImageData& right) const -> ImageData {
        if (left.empty() || right.empty() || left.m_width != right.m_width || left.m_height != right.m_height) {
            return ImageData();
        }

        cv::Mat left_gray, right_gray;
        if (left.m_impl->mat.channels() == 3) {
            cv::cvtColor(left.m_impl->mat, left_gray, cv::COLOR_BGR2GRAY);
        } else {
            left_gray = left.m_impl->mat;
        }
        if (right.m_impl->mat.channels() == 3) {
            cv::cvtColor(right.m_impl->mat, right_gray, cv::COLOR_BGR2GRAY);
        } else {
            right_gray = right.m_impl->mat;
        }

        cv::Mat disparity_mat;
        if (m_impl->config.use_sgbm) {
            m_impl->sgbm->compute(left_gray, right_gray, disparity_mat);
        } else {
            m_impl->bm->compute(left_gray, right_gray, disparity_mat);
        }

        cv::Mat disparity_float;
        disparity_mat.convertTo(disparity_float, CV_32F, 1.0 / 16.0);

        return ImageData::fromRaw(
            reinterpret_cast<const uint8_t*>(disparity_float.data),
            disparity_float.cols, disparity_float.rows,
            PixelFormat::Bgr32F,
            true
        );
    }

    /**
     * @brief Compute 3D point cloud from disparity map
     * @param disparity_map Disparity map
     * @param q_matrix_array Q matrix for reprojection
     * @return 3D point cloud as ImageData
     */
    auto StereoMatching::computeDepth(const ImageData& disparity_map, const calibration::CameraMatrix& q_matrix_array) const -> ImageData {
        if (disparity_map.empty()) {
            return ImageData();
        }

        cv::Mat q_mat(4, 4, CV_64F, const_cast<double*>(q_matrix_array.data()));
        cv::Mat points_3d_mat;
        cv::reprojectImageTo3D(disparity_map.m_impl->mat, points_3d_mat, q_mat, true);

        return ImageData::fromRaw(
            reinterpret_cast<const uint8_t*>(points_3d_mat.data),
            points_3d_mat.cols, points_3d_mat.rows,
            PixelFormat::Bgr32F,
            true
        );
    }

} // namespace perception::stereo