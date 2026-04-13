#include <catch2/catch_test_macros.hpp>
#include <filesystem>

import perception.image_loader;
import perception.geom;
import perception.types;
import perception.async;

using namespace perception;

TEST_CASE("Synthetic frame generation", "[io]") {
    const size_t requested_count = 5;
    auto frames = io::generateTestFrames(requested_count);
    
    size_t actual_count = 0;
    for (const auto& frame : frames) {
        REQUIRE(frame.width == 480);
        REQUIRE(frame.height == 640);
        REQUIRE(frame.channels == 3);
        REQUIRE(frame.data.size() == frame.width * frame.height * frame.channels);
        actual_count++;
    }
    
    CHECK(actual_count == requested_count);
}

TEST_CASE("Non-existent image loading", "[io]") {
    auto result = io::loadImageFile("non_existent_path.jpg");
    CHECK_FALSE(result.has_value());
}

TEST_CASE("Directory discovery", "[io]") {
    // findDemoDataDirectory should at least return a path object (even if empty in restricted CI environments)
    auto path = io::findDemoDataDirectory();
    if (!path.empty()) {
        CHECK(std::filesystem::is_directory(path));
    }
}