#include <catch2/catch_test_macros.hpp>
import perception.detector;

TEST_CASE("Detector - FakeDetections") {
    perception::MockDetector detector;
    // Create a test frame using ImageData (std types only)
    perception::ImageData test_frame(480, 640, 3);

    auto detections = detector.detect(test_frame);

    // Should generate some fake detections
    REQUIRE(detections.size() > 0);

    // Check detection properties
    for (const auto& det : detections) {
        REQUIRE(det.confidence > 0.0f);
        REQUIRE(det.confidence <= 1.0f);
        REQUIRE(det.class_id >= 0);
        REQUIRE(det.bbox.width > 0);
        REQUIRE(det.bbox.height > 0);
        REQUIRE(det.bbox.x >= 0);
        REQUIRE(det.bbox.y >= 0);
        REQUIRE(det.bbox.x + det.bbox.width < test_frame.width);
        REQUIRE(det.bbox.y + det.bbox.height < test_frame.height);
    }
}

TEST_CASE("Detector - EmptyFrame") {
    perception::MockDetector detector;
    perception::ImageData empty_frame;  // Default constructor creates empty frame
    auto detections = detector.detect(empty_frame);

    REQUIRE(detections.size() == 0);
}

// Tests for Tracker, Fusion, and IMU are not available as modules
// TODO: Implement these modules and add corresponding tests
