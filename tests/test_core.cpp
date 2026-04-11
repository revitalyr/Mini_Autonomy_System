#include <gtest/gtest.h>
import perception.queue;
import perception.metrics;
#include <thread>

// Test ThreadSafeQueue
class ThreadSafeQueueTest : public ::testing::Test {
protected:
    perception::ThreadSafeQueue<int> queue_;

    void SetUp() override {
        // Setup if needed
    }
    
    void TearDown() override {
        // Cleanup if needed
    }
};

TEST_F(ThreadSafeQueueTest, BasicOperations) {
    perception::ThreadSafeQueue<int> queue;

    // Test empty queue
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);

    // Test push and pop
    queue.push(42);
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);

    auto value = queue.pop();
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 42);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST_F(ThreadSafeQueueTest, TimeoutOperations) {
    perception::ThreadSafeQueue<int> queue;

    // Test timeout on empty queue
    auto start = std::chrono::steady_clock::now();
    auto value = queue.pop_timeout(std::chrono::milliseconds(50));
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_FALSE(value.has_value());
    EXPECT_GE(duration.count(), 40); // Should wait at least 40ms
}

TEST_F(ThreadSafeQueueTest, ConcurrentOperations) {
    perception::ThreadSafeQueue<int> queue;
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
        while (total_consumed.load() < num_items * num_producers) {
            auto value = queue.pop_timeout(std::chrono::milliseconds(100));
            if (value.has_value()) {
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
    perception::PerformanceMetrics metrics_;
};

TEST_F(MetricsTest, BasicMetrics) {
    // Initial state
    auto snapshot = metrics_.get_snapshot();
    EXPECT_EQ(snapshot.total_frames, 0);
    EXPECT_EQ(snapshot.fps, 0.0);

    // Simulate frame processing
    metrics_.record_frame_latency(33.0);

    snapshot = metrics_.get_snapshot();
    EXPECT_EQ(snapshot.total_frames, 1);
    EXPECT_GT(snapshot.avg_latency_ms, 30.0);
}

TEST_F(MetricsTest, MultipleFrames) {
    const int num_frames = 10;
    const double frame_latency_ms = 16.0; // ~60 FPS

    for (int i = 0; i < num_frames; ++i) {
        metrics_.record_frame_latency(frame_latency_ms);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto snapshot = metrics_.get_snapshot();
    EXPECT_EQ(snapshot.total_frames, num_frames);
    EXPECT_GT(snapshot.fps, 50.0); // Should be around 60 FPS
    EXPECT_LT(snapshot.avg_latency_ms, 20.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
