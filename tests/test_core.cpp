#include <catch2/catch_test_macros.hpp>
import perception.queue;
import perception.metrics;
#include <thread>

TEST_CASE("ThreadSafeQueue - BasicOperations") {
    perception::ThreadSafeQueue<int> queue;

    // Test empty queue
    REQUIRE(queue.empty());
    REQUIRE(queue.size() == 0);

    // Test push and pop
    queue.push(42);
    REQUIRE_FALSE(queue.empty());
    REQUIRE(queue.size() == 1);

    auto value = queue.pop();
    REQUIRE(value.has_value());
    REQUIRE(*value == 42);
    REQUIRE(queue.empty());
    REQUIRE(queue.size() == 0);
}

TEST_CASE("ThreadSafeQueue - TimeoutOperations") {
    perception::ThreadSafeQueue<int> queue;

    // Test timeout on empty queue
    auto start = std::chrono::steady_clock::now();
    auto value = queue.pop_timeout(std::chrono::milliseconds(50));
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    REQUIRE_FALSE(value.has_value());
    REQUIRE(duration.count() >= 40); // Should wait at least 40ms
}

TEST_CASE("ThreadSafeQueue - ConcurrentOperations") {
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

    REQUIRE(total_consumed.load() == num_items * num_producers);
    REQUIRE(queue.empty());
}

TEST_CASE("Metrics - BasicMetrics") {
    perception::PerformanceMetrics metrics;

    // Initial state
    auto snapshot = metrics.get_snapshot();
    REQUIRE(snapshot.total_frames == 0);
    REQUIRE(snapshot.fps == 0.0);

    // Simulate frame processing
    metrics.record_frame_latency(33.0);

    snapshot = metrics.get_snapshot();
    REQUIRE(snapshot.total_frames == 1);
    REQUIRE(snapshot.avg_latency_ms > 30.0);
}

TEST_CASE("Metrics - MultipleFrames") {
    perception::PerformanceMetrics metrics;
    const int num_frames = 10;
    const double frame_latency_ms = 16.0; // ~60 FPS

    for (int i = 0; i < num_frames; ++i) {
        metrics.record_frame_latency(frame_latency_ms);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto snapshot = metrics.get_snapshot();
    REQUIRE(snapshot.total_frames == num_frames);
    REQUIRE(snapshot.fps > 50.0); // Should be around 60 FPS
    REQUIRE(snapshot.avg_latency_ms < 20.0);
}
