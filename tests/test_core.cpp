#include <gtest/gtest.h>
#include "core/thread_safe_queue.hpp"
#include "core/metrics.hpp"
#include "core/config.hpp"
#include <chrono>
#include <thread>

// Test ThreadSafeQueue
class ThreadSafeQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup if needed
    }
    
    void TearDown() override {
        // Cleanup if needed
    }
};

TEST_F(ThreadSafeQueueTest, BasicOperations) {
    ThreadSafeQueue<int> queue;
    
    // Test empty queue
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
    
    // Test push and pop
    queue.push(42);
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);
    
    int value;
    EXPECT_TRUE(queue.pop(value));
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST_F(ThreadSafeQueueTest, TimeoutOperations) {
    ThreadSafeQueue<int> queue;
    
    int value;
    // Test timeout on empty queue
    auto start = std::chrono::steady_clock::now();
    EXPECT_FALSE(queue.pop(value, std::chrono::milliseconds(50)));
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_GE(duration.count(), 40); // Should wait at least 40ms
}

TEST_F(ThreadSafeQueueTest, ConcurrentOperations) {
    ThreadSafeQueue<int> queue;
    const int num_items = 100;
    const int num_producers = 4;
    
    std::vector<std::thread> producers;
    std::atomic<int> total_consumed{0};
    
    // Start producer threads
    for (int p = 0; p < num_producers; ++p) {
        producers.emplace_back([&queue, p, num_items]() {
            for (int i = 0; i < num_items; ++i) {
                queue.push(p * num_items + i);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }
    
    // Consumer thread
    std::thread consumer([&queue, &total_consumed, num_items, num_producers]() {
        int value;
        while (total_consumed.load() < num_items * num_producers) {
            if (queue.pop(value, std::chrono::milliseconds(100))) {
                total_consumed++;
            }
        }
    });
    
    // Wait for completion
    for (auto& producer : producers) {
        producer.join();
    }
    consumer.join();
    
    EXPECT_EQ(total_consumed.load(), num_items * num_producers);
    EXPECT_TRUE(queue.empty());
}

// Test Metrics
class MetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        metrics_ = std::make_unique<Metrics>();
    }
    
    std::unique_ptr<Metrics> metrics_;
};

TEST_F(MetricsTest, BasicMetrics) {
    // Initial state
    EXPECT_EQ(metrics_->get_total_frames(), 0);
    EXPECT_EQ(metrics_->get_fps(), 0.0f);
    
    // Simulate frame processing
    metrics_->frame_start();
    std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
    metrics_->frame_end();
    
    EXPECT_EQ(metrics_->get_total_frames(), 1);
    EXPECT_GT(metrics_->get_last_latency(), 30.0f);
    EXPECT_LT(metrics_->get_fps(), 35.0f); // Should be around 30 FPS
}

TEST_F(MetricsTest, MultipleFrames) {
    const int num_frames = 10;
    const int frame_delay_ms = 16; // ~60 FPS
    
    for (int i = 0; i < num_frames; ++i) {
        metrics_->frame_start();
        std::this_thread::sleep_for(std::chrono::milliseconds(frame_delay_ms));
        metrics_->frame_end();
    }
    
    EXPECT_EQ(metrics_->get_total_frames(), num_frames);
    EXPECT_GT(metrics_->get_fps(), 50.0f); // Should be around 60 FPS
    EXPECT_LT(metrics_->get_last_latency(), 20.0f);
}

// Test Config
class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_ = std::make_unique<Config>();
    }
    
    std::unique_ptr<Config> config_;
};

TEST_F(ConfigTest, BasicOperations) {
    // Test default values
    EXPECT_EQ(config_->get_string("nonexistent", "default"), "default");
    EXPECT_EQ(config_->get_int("nonexistent", 42), 42);
    EXPECT_FLOAT_EQ(config_->get_float("nonexistent", 3.14f), 3.14f);
    EXPECT_FALSE(config_->get_bool("nonexistent", false));
    
    // Test setting values
    config_->set_string("test_string", "hello");
    config_->set_int("test_int", 123);
    config_->set_float("test_float", 45.67f);
    config_->set_bool("test_bool", true);
    
    EXPECT_EQ(config_->get_string("test_string"), "hello");
    EXPECT_EQ(config_->get_int("test_int"), 123);
    EXPECT_FLOAT_EQ(config_->get_float("test_float"), 45.67f);
    EXPECT_TRUE(config_->get_bool("test_bool"));
}

TEST_F(ConfigTest, FileOperations) {
    // Create test config file
    const std::string test_config = 
        "# Test configuration\n"
        "string_key = test_value\n"
        "int_key = 42\n"
        "float_key = 3.14159\n"
        "bool_key_true = true\n"
        "bool_key_false = false\n"
        "bool_key_1 = 1\n"
        "bool_key_0 = 0\n";
    
    std::ofstream file("test_config.txt");
    file << test_config;
    file.close();
    
    // Load configuration
    EXPECT_TRUE(config_->load_from_file("test_config.txt"));
    
    // Test loaded values
    EXPECT_EQ(config_->get_string("string_key"), "test_value");
    EXPECT_EQ(config_->get_int("int_key"), 42);
    EXPECT_FLOAT_EQ(config_->get_float("float_key"), 3.14159f);
    EXPECT_TRUE(config_->get_bool("bool_key_true"));
    EXPECT_FALSE(config_->get_bool("bool_key_false"));
    EXPECT_TRUE(config_->get_bool("bool_key_1"));
    EXPECT_FALSE(config_->get_bool("bool_key_0"));
    
    // Cleanup
    std::remove("test_config.txt");
}

TEST_F(ConfigTest, SaveAndLoad) {
    // Set some values
    config_->set_string("save_test", "value");
    config_->set_int("save_int", 999);
    
    // Save to file
    EXPECT_TRUE(config_->save_to_file("save_test.txt"));
    
    // Create new config and load
    Config new_config;
    EXPECT_TRUE(new_config.load_from_file("save_test.txt"));
    
    // Verify values
    EXPECT_EQ(new_config.get_string("save_test"), "value");
    EXPECT_EQ(new_config.get_int("save_int"), 999);
    
    // Cleanup
    std::remove("save_test.txt");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
