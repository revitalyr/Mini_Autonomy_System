#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
    std::cout << "Simple test started!" << std::endl;
    
    try {
        cv::Mat test(100, 100, CV_8UC3, cv::Scalar(0, 255, 0));
        std::cout << "OpenCV Mat created successfully!" << std::endl;
        std::cout << "Image size: " << test.rows << "x" << test.cols << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Simple test completed!" << std::endl;
    return 0;
}
