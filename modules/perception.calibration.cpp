/**
 * @file perception.calibration.cpp
 * @brief Implementation of camera calibration functions
 */

module;

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <perception/constants.h>

module perception.calibration;

import perception.types;
import perception.result;

namespace perception::calibration {

    using namespace perception::constants;

    /**
     * @struct CalibrationImpl
     * @brief Private implementation details for CameraCalibration
     */
    struct CalibrationImpl {
        cv::Mat k; ///< 3x3 Camera Matrix
        cv::Mat d; ///< Distortion Coefficients
        cv::Mat m_map1, m_map2; ///< Pre-computed rectification maps
        int m_map_width = 0; ///< Width for rectification maps
        int m_map_height = 0; ///< Height for rectification maps
        double m_alpha = 0.0; ///< Free scaling parameter for undistortion
    };

    /**
     * @brief Default constructor
     */
    CameraCalibration::CameraCalibration() 
        : m_impl(std::make_unique<CalibrationImpl>()) {}

    /**
     * @brief Destructor
     */
    CameraCalibration::~CameraCalibration() = default;

    /**
     * @brief Move constructor
     */
    CameraCalibration::CameraCalibration(CameraCalibration&&) noexcept = default;

    /**
     * @brief Move assignment operator
     */
    CameraCalibration& CameraCalibration::operator=(CameraCalibration&&) noexcept = default;

    /**
     * @brief Set camera calibration parameters
     * @param k_data 3x3 camera matrix
     * @param d_data Distortion coefficients
     */
    auto CameraCalibration::setParameters(const CameraMatrix& k_data, const DistortionCoeffs& d_data) -> void {
        m_impl->k = cv::Mat(3, 3, CV_64F, const_cast<double*>(k_data.data())).clone();
        m_impl->d = cv::Mat(1, static_cast<int>(d_data.size()), CV_64F, const_cast<double*>(d_data.data())).clone();
        resetMaps();
    }

    /**
     * @brief Set free scaling parameter for undistortion
     * @param alpha Scaling parameter between 0 and 1
     * @details 0 = all pixels valid, 1 = all original pixels retained
     */
    auto CameraCalibration::setAlpha(double alpha) -> void {
        m_impl->m_alpha = std::clamp(alpha, 0.0, 1.0);
        resetMaps();
    }

    /**
     * @brief Load calibration parameters from YAML file
     * @param filepath Path to YAML file
     * @return Result containing CameraCalibration or error
     */
    auto CameraCalibration::fromYamlFile(const String& filepath) -> Result<CameraCalibration> {
        CameraCalibration calib;
        cv::FileStorage fs(filepath, cv::FileStorage::READ);

        if (!fs.isOpened()) {
            return Result<CameraCalibration>(std::error_code(static_cast<int>(perception::PerceptionError::FileNotFound), std::generic_category()));
        }

        cv::Mat k_mat, d_mat;
        fs["camera_matrix"] >> k_mat;
        fs["distortion_coefficients"] >> d_mat;

        if (k_mat.empty() || d_mat.empty()) {
            return Result<CameraCalibration>(std::error_code(static_cast<int>(perception::PerceptionError::InvalidCalibrationData), std::generic_category()));
        }

        if (k_mat.rows != 3 || k_mat.cols != 3 || k_mat.type() != CV_64F) {
            return Result<CameraCalibration>(std::error_code(static_cast<int>(perception::PerceptionError::InvalidCalibrationData), std::generic_category()));
        }
        CameraMatrix k_array;
        for (int i = 0; i < 9; ++i) {
            k_array[i] = k_mat.at<double>(i / 3, i % 3);
        }

        if (d_mat.rows != 1 || d_mat.type() != CV_64F) {
             return Result<CameraCalibration>(std::error_code(static_cast<int>(perception::PerceptionError::InvalidCalibrationData), std::generic_category()));
        }
        DistortionCoeffs d_vec;
        d_vec.reserve(d_mat.cols);
        for (int i = 0; i < d_mat.cols; ++i) {
            d_vec.push_back(d_mat.at<double>(0, i));
        }

        calib.setParameters(k_array, d_vec);
        return calib;
    }

    /**
     * @brief Save calibration parameters to YAML file
     * @param filepath Path to output YAML file
     * @return Result indicating success or failure
     */
    auto CameraCalibration::saveToYamlFile(const String& filepath) const -> Result<void> {
        if (!isValid()) {
            return Result<void>(std::error_code(static_cast<int>(perception::PerceptionError::InvalidCalibrationData), std::generic_category()));
        }

        try {
            cv::FileStorage fs(filepath, cv::FileStorage::WRITE);
            if (!fs.isOpened()) {
                return Result<void>(std::error_code(static_cast<int>(perception::PerceptionError::IOError), std::generic_category()));
            }
            fs << "camera_matrix" << m_impl->k;
            fs << "distortion_coefficients" << m_impl->d;
            return {};
        } catch (...) {
            return Result<void>(std::error_code(static_cast<int>(perception::PerceptionError::IOError), std::generic_category()));
        }
    }

    /**
     * @brief Reset pre-computed rectification maps
     * @details Invalidates maps to force recomputation on next use
     */
    auto CameraCalibration::resetMaps() -> void {
        if (m_impl) {
            m_impl->m_map1 = cv::Mat();
            m_impl->m_map2 = cv::Mat();
            m_impl->m_map_width = 0;
            m_impl->m_map_height = 0;
        }
    }

    /**
     * @brief Check if calibration parameters are valid
     * @return True if camera matrix and distortion coefficients are set
     */
    auto CameraCalibration::isValid() const noexcept -> bool {
        return m_impl && !m_impl->k.empty() && !m_impl->d.empty();
    }

    /**
     * @brief Undistort an image using camera calibration
     * @param src Source image
     * @return Undistorted image
     * @details Uses pre-computed rectification maps for efficiency
     */
    auto CameraCalibration::undistort(const ImageData& src) const -> ImageData {
        if (src.empty() || !isValid()) {
            return src.clone();
        }

        ImageData result;
        result.m_width = src.m_width;
        result.m_height = src.m_height;
        result.m_channels = src.m_channels;
        result.m_bit_depth = src.m_bit_depth;
        result.m_timestamp = src.m_timestamp;

        if (m_impl->m_map1.empty() || m_impl->m_map_width != src.m_width || m_impl->m_map_height != src.m_height) {
            cv::Size image_size(src.m_width, src.m_height);
            cv::Mat new_k = cv::getOptimalNewCameraMatrix(m_impl->k, m_impl->d, image_size, m_impl->m_alpha, image_size, 0);
            cv::initUndistortRectifyMap(m_impl->k, m_impl->d, cv::Mat(), new_k, image_size, CV_32FC1, m_impl->m_map1, m_impl->m_map2);
            m_impl->m_map_width = src.m_width;
            m_impl->m_map_height = src.m_height;
        }

        result.m_impl->mat.create(src.m_height, src.m_width, src.m_impl->mat.type());
        cv::remap(src.m_impl->mat, result.m_impl->mat, m_impl->m_map1, m_impl->m_map2, cv::INTER_LINEAR);

        return result;
    }

    /**
     * @struct StereoCalibrationImpl
     * @brief Private implementation details for StereoCameraCalibration
     */
    struct StereoCalibrationImpl {
        cv::Mat r, t; ///< Rotation and translation between cameras
        cv::Mat k_left, d_left; ///< Left camera intrinsics
        cv::Mat k_right, d_right; ///< Right camera intrinsics
        cv::Mat p1, p2, q; ///< Projection matrices and disparity-to-depth matrix
        cv::Mat map_l1, map_l2; ///< Left camera rectification maps
        cv::Mat map_r1, map_r2; ///< Right camera rectification maps
        int map_width = 0; ///< Width for rectification maps
        int map_height = 0; ///< Height for rectification maps
    };

    /**
     * @brief Default constructor
     */
    StereoCameraCalibration::StereoCameraCalibration() 
        : m_impl(std::make_unique<StereoCalibrationImpl>()) {}

    /**
     * @brief Destructor
     */
    StereoCameraCalibration::~StereoCameraCalibration() = default;

    /**
     * @brief Move constructor
     */
    StereoCameraCalibration::StereoCameraCalibration(StereoCameraCalibration&&) noexcept = default;

    /**
     * @brief Move assignment operator
     */
    StereoCameraCalibration& StereoCameraCalibration::operator=(StereoCameraCalibration&&) noexcept = default;

    /**
     * @brief Set stereo calibration parameters
     * @param left Left camera calibration
     * @param right Right camera calibration
     * @param r_data Rotation matrix between cameras
     * @param t_data Translation vector between cameras
     */
    auto StereoCameraCalibration::setParameters(
        const CameraCalibration& left, 
        const CameraCalibration& right,
        const RotationMatrix& r_data,
        const TranslationVector& t_data) -> void {

        m_impl->k_left = left.m_impl->k.clone();
        m_impl->d_left = left.m_impl->d.clone();
        m_impl->k_right = right.m_impl->k.clone();
        m_impl->d_right = right.m_impl->d.clone();

        m_impl->r = cv::Mat(3, 3, CV_64F, const_cast<double*>(r_data.data())).clone();
        m_impl->t = cv::Mat(3, 1, CV_64F, const_cast<double*>(t_data.data())).clone();

        m_impl->map_l1 = cv::Mat();
        m_impl->map_width = 0;
    }

    /**
     * @brief Check if stereo calibration is valid
     * @return True if calibration parameters are set
     */
    auto StereoCameraCalibration::isValid() const noexcept -> bool {
        return m_impl && !m_impl->k_left.empty() && !m_impl->r.empty();
    }

    /**
     * @brief Rectify stereo image pair
     * @param left_src Left camera image
     * @param right_src Right camera image
     * @return Pair of rectified images
     */
    auto StereoCameraCalibration::rectifyStereo(const ImageData& left_src, const ImageData& right_src) const -> std::pair<ImageData, ImageData> {
        if (!isValid() || left_src.empty() || right_src.empty()) {
            return {left_src.clone(), right_src.clone()};
        }

        if (m_impl->map_l1.empty() || m_impl->map_width != left_src.m_width || m_impl->map_height != left_src.m_height) {
            cv::Size img_size(left_src.m_width, left_src.m_height);
            cv::Mat r1, r2, p1, p2, q;
            
            cv::stereoRectify(
                m_impl->k_left, m_impl->d_left,
                m_impl->k_right, m_impl->d_right,
                img_size, m_impl->r, m_impl->t,
                r1, r2, p1, p2, q,
                cv::CALIB_ZERO_DISPARITY, 0, img_size
            );
            m_impl->p1 = p1.clone();
            m_impl->p2 = p2.clone();
            m_impl->q = q.clone();

            cv::initUndistortRectifyMap(m_impl->k_left, m_impl->d_left, r1, p1, img_size, CV_32FC1, m_impl->map_l1, m_impl->map_l2);
            cv::initUndistortRectifyMap(m_impl->k_right, m_impl->d_right, r2, p2, img_size, CV_32FC1, m_impl->map_r1, m_impl->map_r2);

            m_impl->map_width = left_src.m_width;
            m_impl->map_height = left_src.m_height;
        }

        auto prepare_result = [](const ImageData& src) {
            ImageData res;
            res.m_width = src.m_width;
            res.m_height = src.m_height;
            res.m_channels = src.m_channels;
            res.m_bit_depth = src.m_bit_depth;
            res.m_timestamp = src.m_timestamp;
            return res;
        };

        ImageData rect_l = prepare_result(left_src);
        ImageData rect_r = prepare_result(right_src);

        rect_l.m_impl->mat.create(left_src.m_height, left_src.m_width, left_src.m_impl->mat.type());
        rect_r.m_impl->mat.create(right_src.m_height, right_src.m_width, right_src.m_impl->mat.type());

        cv::remap(left_src.m_impl->mat, rect_l.m_impl->mat, m_impl->map_l1, m_impl->map_l2, cv::INTER_LINEAR);
        cv::remap(right_src.m_impl->mat, rect_r.m_impl->mat, m_impl->map_r1, m_impl->map_r2, cv::INTER_LINEAR);

        return {std::move(rect_l), std::move(rect_r)};
    }

    /**
     * @brief Get left camera projection matrix
     * @return 3x4 projection matrix
     */
    auto StereoCameraCalibration::getProjectionMatrix1() const -> ProjectionMatrix {
        ProjectionMatrix p_array{};
        if (m_impl && !m_impl->p1.empty()) {
            const double* data = m_impl->p1.ptr<double>();
            std::copy(data, data + 12, p_array.begin());
        }
        return p_array;
    }

    /**
     * @brief Get right camera projection matrix
     * @return 3x4 projection matrix
     */
    auto StereoCameraCalibration::getProjectionMatrix2() const -> ProjectionMatrix {
        ProjectionMatrix p_array{};
        if (m_impl && !m_impl->p2.empty()) {
            const double* data = m_impl->p2.ptr<double>();
            std::copy(data, data + 12, p_array.begin());
        }
        return p_array;
    }

    /**
     * @brief Get disparity-to-depth transformation matrix
     * @return 4x4 Q matrix
     */
    auto StereoCameraCalibration::getDisparityToDepthMatrixQ() const -> std::array<double, 16> {
        std::array<double, 16> q_array{};
        if (m_impl && !m_impl->q.empty()) {
            const double* data = m_impl->q.ptr<double>();
            std::copy(data, data + 16, q_array.begin());
        }
        return q_array;
    }

} // namespace perception::calibration
