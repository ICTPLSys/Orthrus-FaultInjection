#include <immintrin.h>
#include <iostream>

int main() {
    unsigned status = _xbegin();
    if (status == _XBEGIN_STARTED) {
        std::cout << "Transaction started..." << std::endl;

        // 主动触发事务中止，状态码为0x42
        _xabort(0x42);
        
        _xend(); // 不会执行，因为事务被中止
    } else {
        std::cout << "Transaction aborted with status: " << (status & 0xFF) << std::endl;
        if (status & _XABORT_RETRY) {
            std::cout << "Can retry transaction." << std::endl;
        }
        if (status & _XABORT_CAPACITY) {
            std::cout << "Transaction failed due to capacity." << std::endl;
        }
        if (status & _XABORT_CONFLICT) {
            std::cout << "Transaction failed due to conflict." << std::endl;
        }
    }

    return 0;
}
