#include <iostream>
#include <memory>

// Import just one module to test
import perception.queue;

int main() {
    std::cout << "Testing modules..." << std::endl;
    
    try {
        std::cout << "Creating queue..." << std::endl;
        auto queue = std::make_unique<perception::ThreadSafeQueue<int>>();
        std::cout << "Queue created successfully!" << std::endl;
        
        queue->push(42);
        std::cout << "Pushed value 42" << std::endl;
        
        auto result = queue->try_pop();
        if (result) {
            std::cout << "Popped value: " << *result << std::endl;
        }
        
        std::cout << "Module test completed!" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
