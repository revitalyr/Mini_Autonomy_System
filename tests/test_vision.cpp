#include <gtest/gtest.h>
import perception.detector;
import perception.tracker;
import perception.fusion;
#include <opencv2/opencv.hpp>

// Test Detector
class DetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        detector_ = std::make_unique<Detector>("", 0.5f, 0.4f, 640, 640);
        // Create a test frame
        test_frame_ = cv::Mat::zeros(480, 640, CV_8UC3);
        cv::rectangle(test_frame_, cv::Rect(100, 100, 80, 80), cv::Scalar(255, 255, 255), -1);
    }
    
    std::unique_ptr<Detector> detector_;
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

// Test Tracker
class TrackerTest : public ::testing::Test {
protected:
    void SetUp() override {
        tracker_ = std::make_unique<Tracker>(30, 10, 0.3f);
    }
    
    std::unique_ptr<Tracker> tracker_;
};

TEST_F(TrackerTest, BasicTracking) {
    std::vector<Detection> detections;
    detections.emplace_back(cv::Rect(100, 100, 50, 50), 0.9f, 0);
    detections.emplace_back(cv::Rect(200, 200, 60, 60), 0.8f, 1);
    
    auto tracks = tracker_->update(detections);
    
    EXPECT_EQ(tracks.size(), 2);
    
    // Check track properties
    for (const auto& track : tracks) {
        EXPECT_GE(track.id, 0);
        EXPECT_GT(track.bbox.width, 0);
        EXPECT_GT(track.bbox.height, 0);
        EXPECT_EQ(track.age, 1);
        EXPECT_EQ(track.consecutive_misses, 0);
        EXPECT_GT(track.confidence, 0.0f);
    }
}

TEST_F(TrackerTest, TrackContinuity) {
    std::vector<Detection> detections1;
    detections1.emplace_back(cv::Rect(100, 100, 50, 50), 0.9f, 0);
    
    auto tracks1 = tracker_->update(detections1);
    EXPECT_EQ(tracks1.size(), 1);
    int track_id = tracks1[0].id;
    
    // Move detection slightly
    std::vector<Detection> detections2;
    detections2.emplace_back(cv::Rect(105, 105, 50, 50), 0.9f, 0);
    
    auto tracks2 = tracker_->update(detections2);
    EXPECT_EQ(tracks2.size(), 1);
    EXPECT_EQ(tracks2[0].id, track_id); // Same track ID
    EXPECT_GT(tracks2[0].age, 1); // Age should increase
}

TEST_F(TrackerTest, IoUCalculation) {
    cv::Rect box1(100, 100, 50, 50);
    cv::Rect box2(100, 100, 50, 50); // Same box
    cv::Rect box3(150, 150, 50, 50); // No overlap
    cv::Rect box4(125, 125, 50, 50); // Partial overlap
    
    EXPECT_FLOAT_EQ(tracker_->calculate_iou(box1, box2), 1.0f);
    EXPECT_FLOAT_EQ(tracker_->calculate_iou(box1, box3), 0.0f);
    EXPECT_GT(tracker_->calculate_iou(box1, box4), 0.0f);
    EXPECT_LT(tracker_->calculate_iou(box1, box4), 1.0f);
}

// Test IMU Simulator
class IMUTest : public ::testing::Test {
protected:
    void SetUp() override {
        imu_ = std::make_unique<IMUSimulator>(0.01f, 0.5f, 0.1f);
    }
    
    std::unique_ptr<IMUSimulator> imu_;
};

TEST_F(IMUTest, BasicDataGeneration) {
    auto data = imu_->get_data();
    
    // Check that all fields are populated
    EXPECT_TRUE(std::isfinite(data.ax));
    EXPECT_TRUE(std::isfinite(data.ay));
    EXPECT_TRUE(std::isfinite(data.az));
    EXPECT_TRUE(std::isfinite(data.gx));
    EXPECT_TRUE(std::isfinite(data.gy));
    EXPECT_TRUE(std::isfinite(data.gz));
    
    // Check reasonable ranges (accelerometer should include gravity)
    EXPECT_GT(std::abs(data.az), 5.0f); // Should have gravity component
    EXPECT_LT(std::abs(data.ax), 10.0f);
    EXPECT_LT(std::abs(data.ay), 10.0f);
    
    // Gyroscope should be small for simulated motion
    EXPECT_LT(std::abs(data.gx), 2.0f);
    EXPECT_LT(std::abs(data.gy), 2.0f);
    EXPECT_LT(std::abs(data.gz), 2.0f);
}

TEST_F(IMUTest, DataVariation) {
    auto data1 = imu_->get_data();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    auto data2 = imu_->get_data();
    
    // Data should vary over time due to motion simulation
    EXPECT_NE(data1.ax, data2.ax);
    EXPECT_NE(data1.ay, data2.ay);
    EXPECT_NE(data1.gx, data2.gx);
    EXPECT_NE(data1.gy, data2.gy);
}

TEST_F(IMUTest, ParameterConfiguration) {
    imu_->set_noise_level(0.001f); // Lower noise
    imu_->set_motion_parameters(0.1f, 0.05f); // Lower amplitude, higher frequency
    
    auto data = imu_->get_data();
    
    // With lower noise, values should be closer to expected ranges
    EXPECT_GT(std::abs(data.az), 8.0f); // Still has gravity
    EXPECT_LT(std::abs(data.ax), 2.0f); // Lower motion amplitude
    EXPECT_LT(std::abs(data.ay), 2.0f);
}

// Test Fusion
class FusionTest : public ::testing::Test {
protected:
    void SetUp() override {
        fusion_ = std::make_unique<Fusion>();
        imu_ = std::make_unique<IMUSimulator>();
    }
    
    std::unique_ptr<Fusion> fusion_;
    std::unique_ptr<IMUSimulator> imu_;
};

TEST_F(FusionTest, BasicFusion) {
    std::vector<Track> tracks;
    tracks.emplace_back(1, cv::Rect(320, 240, 50, 50), 1.0f, 0.5f);
    
    auto pose = fusion_->update(tracks, imu_->get_data());
    
    // Check that pose is reasonable
    EXPECT_TRUE(std::isfinite(pose.x));
    EXPECT_TRUE(std::isfinite(pose.y));
    EXPECT_TRUE(std::isfinite(pose.z));
    EXPECT_TRUE(std::isfinite(pose.roll));
    EXPECT_TRUE(std::isfinite(pose.pitch));
    EXPECT_TRUE(std::isfinite(pose.yaw));
    
    // Position should be based on track location
    EXPECT_GT(pose.x, 0.0f);
    EXPECT_GT(pose.y, 0.0f);
}

TEST_F(FusionTest, NoTracksFusion) {
    // Test fusion with no tracks (should use IMU only)
    std::vector<Track> tracks;
    
    auto pose1 = fusion_->update(tracks, imu_->get_data());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto pose2 = fusion_->update(tracks, imu_->get_data());
    
    // Position should change based on IMU integration
    EXPECT_NE(pose1.x, pose2.x);
    EXPECT_NE(pose1.y, pose2.y);
    
    // Orientation should change based on gyroscope
    EXPECT_NE(pose1.yaw, pose2.yaw);
}

TEST_F(FusionTest, ParameterConfiguration) {
    fusion_->set_process_noise(0.005f, 0.05f, 0.005f);
    fusion_->set_measurement_noise(0.05f, 0.25f, 0.05f);
    
    std::vector<Track> tracks;
    tracks.emplace_back(1, cv::Rect(320, 240, 50, 50), 1.0f, 0.5f);
    
    auto pose = fusion_->update(tracks, imu_->get_data());
    
    // Should still work with new parameters
    EXPECT_TRUE(std::isfinite(pose.x));
    EXPECT_TRUE(std::isfinite(pose.y));
    EXPECT_TRUE(std::isfinite(pose.z));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
