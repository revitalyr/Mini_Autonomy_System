#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "perception/detector.hpp"

// Test Detector
class DetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        detector_ = std::make_unique<perception::MockDetector>();
        // Create a test frame
        test_frame_ = cv::Mat::zeros(480, 640, CV_8UC3);
        cv::rectangle(test_frame_, cv::Rect(100, 100, 80, 80), cv::Scalar(255, 255, 255), -1);
    }

    std::unique_ptr<perception::MockDetector> detector_;
    cv::Mat test_frame_;
};

TEST_F(DetectorTest, FakeDetections) {
    auto detections = detector_->detect(test_frame_);

    // Should generate some fake detections
    EXPECT_GT(detections.size(), 0);

    // Check detection properties
    for (const auto& det : detections) {
        EXPECT_GT(det.confidence, 0.0f);
        EXPECT_LE(det.confidence, 1.0f);
        EXPECT_GE(det.class_id, 0);
        EXPECT_GT(det.bbox.width, 0);
        EXPECT_GT(det.bbox.height, 0);
        EXPECT_GE(det.bbox.x, 0);
        EXPECT_GE(det.bbox.y, 0);
        EXPECT_LT(det.bbox.x + det.bbox.width, test_frame_.cols);
        EXPECT_LT(det.bbox.y + det.bbox.height, test_frame_.rows);
    }
}

TEST_F(DetectorTest, EmptyFrame) {
    cv::Mat empty_frame;
    auto detections = detector_->detect(empty_frame);

    EXPECT_EQ(detections.size(), 0);
}

// Tests for Tracker, Fusion, and IMU are not available as modules
// TODO: Implement these modules and add corresponding tests

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
