#include <gtest/gtest.h>
import perception.detector;

TEST(DetectorTest, FakeDetections) {
    perception::MockDetector detector;
    // Create a test frame using ImageData (std types only)
    perception::ImageData test_frame(480, 640, 3);

    auto detections = detector.detect(test_frame);

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
        EXPECT_LT(det.bbox.x + det.bbox.width, test_frame.width);
        EXPECT_LT(det.bbox.y + det.bbox.height, test_frame.height);
    }
}

TEST(DetectorTest, EmptyFrame) {
    perception::MockDetector detector;
    perception::ImageData empty_frame;  // Default constructor creates empty frame
    auto detections = detector.detect(empty_frame);

    EXPECT_EQ(detections.size(), 0);
}

// Tests for Tracker, Fusion, and IMU are not available as modules
// TODO: Implement these modules and add corresponding tests
