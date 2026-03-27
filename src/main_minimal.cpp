#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

int main() {
    std::cout << "=== Minimal Test ===" << std::endl;
    std::cout << "Program started!" << std::endl;
    
    // Simple test to see if we can get any output
    for (int i = 0; i < 5; ++i) {
        std::cout << "Count: " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Minimal test completed!" << std::endl;
    return 0;
}
