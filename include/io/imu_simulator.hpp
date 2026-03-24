#pragma once

#include <random>
#include <chrono>

struct IMUData {
    float ax, ay, az;  // Accelerometer (m/s^2)
    float gx, gy, gz;  // Gyroscope (rad/s)
    float mx, my, mz;  // Magnetometer (optional)
    std::chrono::steady_clock::time_point timestamp;
    
    IMUData() : ax(0), ay(0), az(0), gx(0), gy(0), gz(0), mx(0), my(0), mz(0), 
                timestamp(std::chrono::steady_clock::now()) {}
                
    IMUData(float ax_, float ay_, float az_, float gx_, float gy_, float gz_)
        : ax(ax_), ay(ay_), az(az_), gx(gx_), gy(gy_), gz(gz_), mx(0), my(0), mz(0),
          timestamp(std::chrono::steady_clock::now()) {}
};

class IMUSimulator {
private:
    std::default_random_engine engine_;
    std::normal_distribution<float> accel_noise_;
    std::normal_distribution<float> gyro_noise_;
    std::normal_distribution<float> bias_drift_;
    
    // Simulated motion parameters
    float motion_amplitude_;
    float motion_frequency_;
    std::chrono::steady_clock::time_point start_time_;
    
    // IMU biases
    float accel_bias_x_, accel_bias_y_, accel_bias_z_;
    float gyro_bias_x_, gyro_bias_y_, gyro_bias_z_;
    
public:
    IMUSimulator(float noise_std = 0.01f, float motion_amplitude = 0.5f, float motion_frequency = 0.1f);
    
    // Get simulated IMU data
    IMUData get_data();
    
    // Configure simulation parameters
    void set_noise_level(float std_dev);
    void set_motion_parameters(float amplitude, float frequency);
    void set_biases(float accel_bias_x, float accel_bias_y, float accel_bias_z,
                   float gyro_bias_x, float gyro_bias_y, float gyro_bias_z);
    
    // Reset simulation
    void reset();
    
private:
    float generate_motion_value(float amplitude, float frequency, float phase) const;
};
