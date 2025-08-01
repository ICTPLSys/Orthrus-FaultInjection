#include <iostream>
#include <random>

void branch_1() {
    std::random_device rd;
    std::mt19937 gen(rd());
    // gen int between 0 and 10
    std::uniform_int_distribution<> dis(0, 10);

    int a = dis(gen);
    int b = dis(gen);
    int c = dis(gen);

    if (a > b) {
        c = a * b;
    } else {
        c = a / b;
    }

    std::cout << c << std::endl;
}

int main() {
    branch_1();
    return 0;
    // std::random_device rd;
    // std::mt19937 gen(rd());
    // // gen int between 0 and 10
    // std::uniform_int_distribution<> dis(0, 10);

    // int a = dis(gen);
    // int b = dis(gen);
    // int c = dis(gen);

    // if (a > b) {
    //     c = a + b;
    // } else {
    //     c = a - b;
    // }

    // std::cout << c << std::endl;
}