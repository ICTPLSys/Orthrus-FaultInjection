// Write a test porgram in C++ by mutex to test cache coherence
// The program will create two threads, one thread will write to a shared variable, the other thread will read from the shared variable
// The program will use mutex to synchronize the access to the shared variable
// The program will print the value of the shared variable
// write first, then read

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

int shared_variable = 0;
std::mutex mtx;

__attribute__((noinline)) void writer(int i) {
    // mtx.lock();
    shared_variable = i;
    // std::cout << "Writer: " << shared_variable << std::endl;
    // mtx.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

__attribute__((noinline)) void reader() {
    // mtx.lock();
    std::cout << "Reader: " << shared_variable << std::endl;
    // mtx.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

int main() {

    for (int i = 0; i < 10; i++) {
        std::thread writer_thread(writer, i);
        writer_thread.join();

        std::thread reader_thread(reader);
        reader_thread.join();
    }
    return 0;
}