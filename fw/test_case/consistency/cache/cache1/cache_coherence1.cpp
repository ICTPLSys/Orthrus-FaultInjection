#include <atomic>
#include <thread>
#include <iostream>
#include <vector>
#include <immintrin.h> // 包含 _mm_lfence()
#include <cstdlib>
#include <random>
std::atomic<int> shared_data(0);

// a vector of atomic variables
std::vector<std::atomic<int>> shared_data_vector;

void reader() {
    // int value = shared_data.load(std::memory_order_acquire);
    int value = shared_data.load();
    std::cout << "Read data: " << value << std::endl;
}

void writer(int value1, int value2) {
    int value = value1 + value2;
    shared_data.store(value);
}

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 99);

    int random_value1 = distrib(gen);
    int random_value2 = distrib(gen);
    std::cout << "Random value1: " << random_value1 << std::endl;
    std::cout << "Random value2: " << random_value2 << std::endl;

    std::thread t1(writer, random_value1, random_value2);
    t1.join();

    std::thread t2(reader);
    t2.join();
}
