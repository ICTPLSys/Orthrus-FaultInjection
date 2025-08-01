#pragma once

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif
#ifndef very_likely
#define very_likely(x) __builtin_expect_with_probability(!!(x), 1, 1.0)
#endif
#ifndef very_unlikely
#define very_unlikely(x) __builtin_expect_with_probability(!!(x), 0, 1.0)
#endif
#define unreachable() __builtin_unreachable()

inline void cpu_relax() { __asm__ __volatile__("pause\n" : : : "memory"); }
