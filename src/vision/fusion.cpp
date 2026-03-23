#include "fusion.hpp"
#include <cmath>
#include <iostream>

Fusion::Fusion(float process_noise_pos, 
               float process_noise_vel,
               float process_noise_angle,
               float measurement_noise_pos,
               float measurement_noise_vel,
               float measurement_noise_angle)
    : process_noise_pos_(process_noise_pos),
      process_noise_vel_(process_noise_vel),
      process_noise_angle_(process_noise_angle),
      measurement_noise_pos_(measurement_noise_pos),
      measurement_noise_vel_(measurement_noise_vel),
      measurement_noise_angle_(measurement_noise_angle),
      last_update_(std::chrono::steady_clock::now()) {
    
    // Initialize state vector [x, y, z, vx, vy, vz, roll, pitch, yaw]
    state_.resize(9, 0.0f);
    
    // Initialize covariance matrix
    initialize_matrices();
}

Pose Fusion::update(const std::vector<Track>& tracks, const IMUData& imu) {
    // For demo, use simple fusion
    return simple_fusion(tracks, imu);
}

void Fusion::predict(const IMUData& imu, float dt) {
    // Simple prediction using IMU acceleration
    state_[0] += state_[3] * dt + 0.5f * imu.ax * dt * dt; // x += vx*dt + 0.5*ax*dt^2
    state_[1] += state_[4] * dt + 0.5f * imu.ay * dt * dt; // y += vy*dt + 0.5*ay*dt^2
    state_[2] += state_[5] * dt + 0.5f * imu.az * dt * dt; // z += vz*dt + 0.5*az*dt^2
    
    state_[3] += imu.ax * dt; // vx += ax*dt
    state_[4] += imu.ay * dt; // vy += ay*dt
    state_[5] += imu.az * dt; // vz += az*dt
    
    // Update orientation (simplified)
    state_[6] += imu.gx * dt; // roll += gx*dt
    state_[7] += imu.gy * dt; // pitch += gy*dt
    state_[8] += imu.gz * dt; // yaw += gz*dt
}

void Fusion::update_visual(const std::vector<Track>& tracks) {
    if (tracks.empty()) return;
    
    // Extract position from tracks (simple centroid)
    Pose visual_pose = extract_pose_from_tracks(tracks);
    
    // Simple update: blend state with visual measurement
    float alpha = 0.3f; // Fusion factor
    state_[0] = (1.0f - alpha) * state_[0] + alpha * visual_pose.x;
    state_[1] = (1.0f - alpha) * state_[1] + alpha * visual_pose.y;
    state_[2] = (1.0f - alpha) * state_[2] + alpha * visual_pose.z;
    
    // Extract velocity from tracks
    auto velocity = extract_velocity_from_tracks(tracks);
    float alpha_vel = 0.2f;
    state_[3] = (1.0f - alpha_vel) * state_[3] + alpha_vel * velocity.first;
    state_[4] = (1.0f - alpha_vel) * state_[4] + alpha_vel * velocity.second;
}

Pose Fusion::simple_fusion(const std::vector<Track>& tracks, const IMUData& imu) {
    Pose result;
    
    if (!tracks.empty()) {
        // Extract position from tracks
        Pose visual_pose = extract_pose_from_tracks(tracks);
        
        // Simple weighted fusion
        float visual_weight = 0.7f;
        float imu_weight = 0.3f;
        
        result.x = visual_weight * visual_pose.x + imu_weight * 0.0f; // IMU provides relative motion
        result.y = visual_weight * visual_pose.y + imu_weight * 0.0f;
        result.z = visual_weight * visual_pose.z + imu_weight * 0.0f;
        
        // Extract velocity
        auto velocity = extract_velocity_from_tracks(tracks);
        result.vx = visual_weight * velocity.first + imu_weight * imu.ax * 0.1f;
        result.vy = visual_weight * velocity.second + imu_weight * imu.ay * 0.1f;
        result.vz = visual_weight * 0.0f + imu_weight * imu.az * 0.1f;
        
        // Orientation from IMU (simplified)
        result.roll = imu.gx * 0.1f;
        result.pitch = imu.gy * 0.1f;
        result.yaw = imu.gz * 0.1f;
        
        // Angular velocity from IMU
        result.wx = imu.gx;
        result.wy = imu.gy;
        result.wz = imu.gz;
    } else {
        // No tracks, rely on IMU only
        static float x = 0.0f, y = 0.0f, z = 0.0f;
        static float vx = 0.0f, vy = 0.0f, vz = 0.0f;
        static float roll = 0.0f, pitch = 0.0f, yaw = 0.0f;
        
        float dt = 0.033f; // Assume 30 FPS
        
        // Integrate IMU data
        vx += imu.ax * dt;
        vy += imu.ay * dt;
        vz += imu.az * dt;
        
        x += vx * dt;
        y += vy * dt;
        z += vz * dt;
        
        roll += imu.gx * dt;
        pitch += imu.gy * dt;
        yaw += imu.gz * dt;
        
        result.x = x;
        result.y = y;
        result.z = z;
        result.vx = vx;
        result.vy = vy;
        result.vz = vz;
        result.roll = roll;
        result.pitch = pitch;
        result.yaw = yaw;
        result.wx = imu.gx;
        result.wy = imu.gy;
        result.wz = imu.gz;
    }
    
    return result;
}

Pose Fusion::get_pose() const {
    Pose pose;
    pose.x = state_[0];
    pose.y = state_[1];
    pose.z = state_[2];
    pose.vx = state_[3];
    pose.vy = state_[4];
    pose.vz = state_[5];
    pose.roll = state_[6];
    pose.pitch = state_[7];
    pose.yaw = state_[8];
    return pose;
}

void Fusion::reset() {
    std::fill(state_.begin(), state_.end(), 0.0f);
    initialize_matrices();
    last_update_ = std::chrono::steady_clock::now();
}

void Fusion::set_process_noise(float pos_noise, float vel_noise, float angle_noise) {
    process_noise_pos_ = pos_noise;
    process_noise_vel_ = vel_noise;
    process_noise_angle_ = angle_noise;
    initialize_matrices();
}

void Fusion::set_measurement_noise(float pos_noise, float vel_noise, float angle_noise) {
    measurement_noise_pos_ = pos_noise;
    measurement_noise_vel_ = vel_noise;
    measurement_noise_angle_ = angle_noise;
    initialize_matrices();
}

void Fusion::initialize_matrices() {
    // Initialize covariance matrix P (9x9)
    P_.resize(9, std::vector<float>(9, 0.0f));
    for (int i = 0; i < 9; ++i) {
        P_[i][i] = 1.0f; // Identity matrix
    }
    
    // Initialize process noise matrix Q (9x9)
    Q_.resize(9, std::vector<float>(9, 0.0f));
    Q_[0][0] = Q_[1][1] = Q_[2][2] = process_noise_pos_;
    Q_[3][3] = Q_[4][4] = Q_[5][5] = process_noise_vel_;
    Q_[6][6] = Q_[7][7] = Q_[8][8] = process_noise_angle_;
    
    // Initialize measurement noise matrix R (9x9)
    R_.resize(9, std::vector<float>(9, 0.0f));
    R_[0][0] = R_[1][1] = R_[2][2] = measurement_noise_pos_;
    R_[3][3] = R_[4][4] = R_[5][5] = measurement_noise_vel_;
    R_[6][6] = R_[7][7] = R_[8][8] = measurement_noise_angle_;
}

Pose Fusion::extract_pose_from_tracks(const std::vector<Track>& tracks) {
    Pose pose;
    
    if (tracks.empty()) {
        return pose;
    }
    
    // Calculate centroid of all tracks
    float sum_x = 0.0f, sum_y = 0.0f;
    for (const auto& track : tracks) {
        cv::Point center(track.bbox.x + track.bbox.width / 2,
                        track.bbox.y + track.bbox.height / 2);
        sum_x += center.x;
        sum_y += center.y;
    }
    
    float avg_x = sum_x / tracks.size();
    float avg_y = sum_y / tracks.size();
    
    // Convert pixel coordinates to meters (assume 1 pixel = 0.01m for demo)
    pose.x = avg_x * 0.01f;
    pose.y = avg_y * 0.01f;
    pose.z = 0.0f; // No depth information
    
    return pose;
}

std::pair<float, float> Fusion::extract_velocity_from_tracks(const std::vector<Track>& tracks) {
    float avg_vx = 0.0f, avg_vy = 0.0f;
    
    if (!tracks.empty()) {
        for (const auto& track : tracks) {
            avg_vx += track.vx;
            avg_vy += track.vy;
        }
        avg_vx /= tracks.size();
        avg_vy /= tracks.size();
        
        // Convert pixel/frame to m/s (assume 30 FPS)
        avg_vx = avg_vx * 0.01f * 30.0f;
        avg_vy = avg_vy * 0.01f * 30.0f;
    }
    
    return std::make_pair(avg_vx, avg_vy);
}
