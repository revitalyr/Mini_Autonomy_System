#include "thread_safe_queue.hpp"

// Explicit template instantiation for common types
template class ThreadSafeQueue<int>;
template class ThreadSafeQueue<float>;
template class ThreadSafeQueue<double>;
template class ThreadSafeQueue<std::string>;

// Add more template instantiations as needed
