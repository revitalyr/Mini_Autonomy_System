#pragma once

#include "tracker.hpp"
#include "../io/imu_simulator.hpp"
#include <vector>

struct Pose {
    float x, y, z;        // Position (meters)
    float roll, pitch, yaw; // Orientation (radians)
    float vx, vy, vz;     // Linear velocity (m/s)
    float wx, wy, wz;     // Angular velocity (rad/s)
    
    Pose() : x(0), y(0), z(0), roll(0), pitch(0), yaw(0), vx(0), vy(0), vz(0), wx(0), wy(0), wz(0) {}
    
    Pose(float pos_x, float pos_y, float pos_z, float orientation_yaw = 0.0f)
        : x(pos_x), y(pos_y), z(pos_z), roll(0), pitch(0), yaw(orientation_yaw), 
          vx(0), vy(0), vz(0), wx(0), wy(0), wz(0) {}
};

class Fusion {
private:
    // Extended Kalman Filter state: [x, y, z, vx, vy, vz, roll, pitch, yaw]
    std::vector<float> state_;           // State vector (9 elements)
    std::vector<std::vector<float>> P_;  // Covariance matrix (9x9)
    std::vector<std::vector<float>> Q_;  // Process noise matrix (9x9)
    std::vector<std::vector<float>> R_;  // Measurement noise matrix (9x9)
    
    // Filter parameters
    float process_noise_pos_;     // Position process noise
    float process_noise_vel_;     // Velocity process noise
    float process_noise_angle_;   // Angle process noise
    float measurement_noise_pos_; // Position measurement noise
    float measurement_noise_vel_; // Velocity measurement noise
    float measurement_noise_angle_; // Angle measurement noise
    
    // Timing
    std::chrono::steady_clock::time_point last_update_;
    
public:
    Fusion(float process_noise_pos = 0.01f, 
           float process_noise_vel = 0.1f,
           float process_noise_angle = 0.01f,
           float measurement_noise_pos = 0.1f,
           float measurement_noise_vel = 0.5f,
           float measurement_noise_angle = 0.1f);
    
    // Main fusion update
    Pose update(const std::vector<Track>& tracks, const IMUData& imu);
    
    // Prediction step (using IMU)
    void predict(const IMUData& imu, float dt);
    
    // Update step (using visual tracking)
    void update_visual(const std::vector<Track>& tracks);
    
    // Simple fusion (for demo)
    Pose simple_fusion(const std::vector<Track>& tracks, const IMUData& imu);
    
    // Get current pose estimate
    Pose get_pose() const;
    
    // Reset filter
    void reset();
    
    // Configure filter parameters
    void set_process_noise(float pos_noise, float vel_noise, float angle_noise);
    void set_measurement_noise(float pos_noise, float vel_noise, float angle_noise);
    
private:
    // EKF helper functions
    void initialize_matrices();
    void predict_step(float dt);
    void update_step(const std::vector<float>& measurement, 
                     const std::vector<std::vector<float>>& H);
    
    // Matrix operations (simplified)
    std::vector<std::vector<float>> matrix_multiply(
        const std::vector<std::vector<float>>& A,
        const std::vector<std::vector<float>>& B);
    
    std::vector<std::vector<float>> matrix_transpose(
        const std::vector<std::vector<float>>& A);
    
    std::vector<std::vector<float>> matrix_inverse(
        const std::vector<std::vector<float>>& A);
    
    // Extract pose from tracks (simple centroid approach)
    Pose extract_pose_from_tracks(const std::vector<Track>& tracks);
    
    // Extract velocity from tracks
    std::pair<float, float> extract_velocity_from_tracks(const std::vector<Track>& tracks);
};
