find_package(Catch2 3 REQUIRED)

add_executable(unit_tests
    test_image_loader.cpp
)

target_link_libraries(unit_tests PRIVATE 
    Catch2::Catch2WithMain 
    perception_modules
    ${OpenCV_LIBS}
)