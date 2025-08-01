#include <cctype>
#include <cstdio>
#include <cstring>
#include <string_view>
#include <utility>
#include <vector>

#include "scee.hpp"
#include "thread.hpp"
#include "wc.hpp"

namespace raw {
#include "closure.hpp"
}  // namespace raw
namespace app {
#include "closure.hpp"
}  // namespace app
namespace validator {
#include "closure.hpp"
}  // namespace validator

constexpr size_t NumMappers = 4;
constexpr size_t NumReducers = 4;
constexpr size_t NumBuckets = 8;

int main_fn(scee::imm_array<scee::mut_pointer<hash_table>> containers) {
    const char example[] =
        "five seven three six five four five seven seven five three six seven "
        "six six seven two seven two four four three five six six four seven "
        "one";
    const size_t example_size = std::strlen(example);
    printf("example size: %lu\n", example_size);

    // split
    std::string_view splits[NumMappers];
    size_t split_size = example_size / NumMappers;
    const char *const end = example + example_size;
    const char *cursor = example;
    for (size_t i = 0; i < NumMappers; ++i) {
        const char *const split_start = cursor;
        const char *split_end;
        if (i == NumMappers - 1) {
            split_end = end;
        } else {
            split_end = cursor + split_size;
            while (split_end < end && !std::isspace(*split_end)) {
                ++split_end;
            }
        }
        splits[i] = std::string_view(split_start, split_end - split_start);
        cursor = split_end;
    }

    for (size_t i = 0; i < NumMappers; ++i) {
        __scee_run(word_count_map_worker, splits[i], containers, i,
                   NumReducers);
    }

    std::vector<trivial_pair<scee::imm_array<kv_pair>, size_t>> results;
    results.reserve(NumReducers);
    for (size_t i = 0; i < NumReducers; ++i) {
        auto [result, result_size] =
            __scee_run(word_count_reduce_worker, containers, NumMappers, i);
        printf("reduce result %lu:\n", i);
        const auto *r = result.deref_nullable();
        for (size_t i = 0; i < result_size; ++i) {
            printf("%8.*s: %lu\t", (int)r[i].key.size, r[i].key.data.deref(),
                   r[i].value);
        }
        printf("\n");
        results.emplace_back(result, result_size);
    }
    const auto *results_ptr = &results;
    auto [final_results, final_result_size] =
        __scee_run(sort_results, results_ptr);
    printf("final results:\n");
    const auto *fr = final_results.deref_nullable();
    for (size_t i = 0; i < final_result_size; ++i) {
        printf("%8.*s: %lu\t", (int)fr[i].key.size, fr[i].key.data.deref(),
               fr[i].value);
    }
    printf("\n");

    final_results.destroy();
    for (auto &result : results) {
        result.first.destroy();
    }
    return 0;
}

int main() {
    auto containers =
        raw::create_containers(NumMappers, NumReducers, NumBuckets);
    scee::main_thread(main_fn, containers);
    raw::destroy_containers(containers, NumMappers, NumReducers);
    return 0;
}