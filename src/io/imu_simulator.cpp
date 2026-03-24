#include "imu_simulator.hpp"
#include <cmath>

IMUSimulator::IMUSimulator(float noise_std, float motion_amplitude, float motion_frequency)
    : engine_(std::random_device{}()),
      accel_noise_(0.0f, noise_std),
      gyro_noise_(0.0f, noise_std * 0.1f),  // Gyro noise typically smaller
      bias_drift_(0.0f, noise_std * 0.01f),  // Slow bias drift
      motion_amplitude_(motion_amplitude),
      motion_frequency_(motion_frequency),
      start_time_(std::chrono::steady_clock::now()),
      accel_bias_x_(0.0f), accel_bias_y_(0.0f), accel_bias_z_(0.0f),
      gyro_bias_x_(0.0f), gyro_bias_y_(0.0f), gyro_bias_z_(0.0f) {
}

IMUData IMUSimulator::get_data() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
    float t = elapsed.count() / 1000.0f;  // Convert to seconds
    
    IMUData data;
    
    // Simulate motion (simple sinusoidal pattern)
    data.ax = generate_motion_value(motion_amplitude_, motion_frequency_, t) + accel_bias_x_ + accel_noise_(engine_);
    data.ay = generate_motion_value(motion_amplitude_ * 0.7f, motion_frequency_ * 1.3f, t + M_PI/3) + accel_bias_y_ + accel_noise_(engine_);
    data.az = -9.81f + generate_motion_value(motion_amplitude_ * 0.3f, motion_frequency_ * 0.7f, t + M_PI/6) + accel_bias_z_ + accel_noise_(engine_);  // Include gravity
    
    data.gx = generate_motion_value(motion_amplitude_ * 0.5f, motion_frequency_ * 2.0f, t) + gyro_bias_x_ + gyro_noise_(engine_);
    data.gy = generate_motion_value(motion_amplitude_ * 0.4f, motion_frequency_ * 1.5f, t + M_PI/4) + gyro_bias_y_ + gyro_noise_(engine_);
    data.gz = generate_motion_value(motion_amplitude_ * 0.6f, motion_frequency_ * 1.8f, t + M_PI/2) + gyro_bias_z_ + gyro_noise_(engine_);
    
    // Simulate magnetometer (optional, simplified)
    data.mx = 0.3f + accel_noise_(engine_) * 0.1f;
    data.my = -0.1f + accel_noise_(engine_) * 0.1f;
    data.mz = 0.5f + accel_noise_(engine_) * 0.1f;
    
    data.timestamp = now;
    
    // Simulate slow bias drift
    accel_bias_x_ += bias_drift_(engine_);
    accel_bias_y_ += bias_drift_(engine_);
    accel_bias_z_ += bias_drift_(engine_);
    gyro_bias_x_ += bias_drift_(engine_);
    gyro_bias_y_ += bias_drift_(engine_);
    gyro_bias_z_ += bias_drift_(engine_);
    
    return data;
}

void IMUSimulator::set_noise_level(float std_dev) {
    accel_noise_ = std::normal_distribution<float>(0.0f, std_dev);
    gyro_noise_ = std::normal_distribution<float>(0.0f, std_dev * 0.1f);
    bias_drift_ = std::normal_distribution<float>(0.0f, std_dev * 0.01f);
}

void IMUSimulator::set_motion_parameters(float amplitude, float frequency) {
    motion_amplitude_ = amplitude;
    motion_frequency_ = frequency;
}

void IMUSimulator::set_biases(float accel_bias_x, float accel_bias_y, float accel_bias_z,
                             float gyro_bias_x, float gyro_bias_y, float gyro_bias_z) {
    accel_bias_x_ = accel_bias_x;
    accel_bias_y_ = accel_bias_y;
    accel_bias_z_ = accel_bias_z;
    gyro_bias_x_ = gyro_bias_x;
    gyro_bias_y_ = gyro_bias_y;
    gyro_bias_z_ = gyro_bias_z;
}

void IMUSimulator::reset() {
    start_time_ = std::chrono::steady_clock::now();
    accel_bias_x_ = accel_bias_y_ = accel_bias_z_ = 0.0f;
    gyro_bias_x_ = gyro_bias_y_ = gyro_bias_z_ = 0.0f;
}

float IMUSimulator::generate_motion_value(float amplitude, float frequency, float phase) const {
    return amplitude * std::sin(2.0f * M_PI * frequency * phase + phase);
}
