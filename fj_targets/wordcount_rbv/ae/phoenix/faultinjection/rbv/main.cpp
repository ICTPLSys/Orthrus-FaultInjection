#include <boost/program_options.hpp>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <utility>
#include <thread>
#include <future>

#include <span>

#include "phoenix.hpp"
#include "scee.hpp"
#include "thread.hpp"
#include "load_file.hpp"
#ifdef PROFILE_MEM
#include "profile-mem.hpp"
#endif

namespace raw {
#include "closure.hpp"
}  // namespace raw
namespace app {
#include "closure.hpp"
}  // namespace app
namespace validator {
#include "closure.hpp"
}  // namespace validator


struct Args {
    std::string input_file;
    map_reduce_config config;
};

Args parse_args(int argc, char **argv) {
    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "produce help message")
        ("input,i", po::value<std::string>()->required(), "input file")
        ("num-buckets,b", po::value<size_t>()->default_value(8 * 1024),
         "number of buckets")
        ("num-mappers,m", po::value<size_t>()->default_value(3 * 1024),
         "number of mappers")
        ("num-reducers,r", po::value<size_t>()->default_value(128),
         "number of reducers")
        ("num-threads,t", po::value<size_t>()->default_value(16),
         "number of threads");
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        exit(0);
    }

    return Args{
            vm["input"].as<std::string>(),
            {
                .n_mappers = vm["num-mappers"].as<size_t>(),
                .n_reducers = vm["num-reducers"].as<size_t>(),
                .n_buckets = vm["num-buckets"].as<size_t>(),
                .n_threads = vm["num-threads"].as<size_t>(),
            }};
}

#ifdef __scee_run
#undef __scee_run
#endif
#define __scee_run(fn, args...)   \
  ([&]() {                         \
    if constexpr (isMain) {       \
      return app::fn(args);       \
    } else {                      \
      return validator::fn(args); \
    }                             \
  })()


namespace NO_FAULT_INJECTION {

#define ASSERT_EQ(val1, val2)                                              \
  do {                                                                     \
    if ((val1) != (val2)) {                                                \
      std::cerr << "Validation failed: ASSERT_EQ(N/A, N/A)\n";             \
      std::cerr << "File: " << __FILE__ << ", Line: " << __LINE__ << "\n"; \
      abort();                                                             \
    }                                                                      \
  } while (0)

#define ASSERT_EQ_FINAL(val1, val2)                                        \
  do {                                                                     \
    if ((val1) != (val2)) {                                                \
      std::cerr << "SDC Not Detected Test failed: ASSERT_EQ(N/A, N/A)\n";  \
      std::cerr << "File: " << __FILE__ << ", Line: " << __LINE__ << "\n"; \
      abort();                                                             \
    }                                                                      \
  } while (0)

struct Results {
    // internal states
    std::vector<std::string_view> splits;
    result_t map_results[255];
    result_t reduce_results[255];
    double wf;
    double average_length;
    double average_frequency;
    double standard_deviation;
    uint32_t val_crc;

    // final results
    int final_result_size;
    std::vector<std::pair<std::string, uint32_t>> final_results;

    int ret;

    friend bool post_check(const Results& ret1, const Results& ret2, const map_reduce_config& config) {
        // RBV SDC CHECK
        auto cmp_f = [](word a, word b) {
            ASSERT_EQ(a.size, b.size);
            auto size = a.size;
            auto arr1 = std::span {a.data.deref_nullable(), size};
            auto arr2 = std::span {b.data.deref_nullable(), size};
            for (auto i = 0; i < size; i++) {
                ASSERT_EQ(arr1[i], arr2[i]);
            }
        };

        ASSERT_EQ(ret1.splits, ret2.splits);
        for (int i = 0; i < config.n_threads; i++) {
            ASSERT_EQ(ret1.map_results[i].second, ret2.map_results[i].second);
            auto size = ret1.map_results[i].second;

            auto arr1 = std::span {ret1.map_results[i].first.deref_nullable(), size};
            auto arr2 = std::span {ret2.map_results[i].first.deref_nullable(), size};
            for (int j = 0; j < size; j++) {
                cmp_f(arr1[j].key, arr2[j].key);
                ASSERT_EQ(arr1[j].value, arr2[j].value);
            }
            // ASSERT_EQ(arr1, arr2);
        }
        for (int i = 0; i < config.n_threads; i++) {
            ASSERT_EQ(ret1.reduce_results[i].second, ret2.reduce_results[i].second);
            auto size = ret1.reduce_results[i].second;

            auto arr1 = std::span {ret1.reduce_results[i].first.deref_nullable(), size};
            auto arr2 = std::span {ret2.reduce_results[i].first.deref_nullable(), size};
            for (int j = 0; j < size; j++) {
                cmp_f(arr1[j].key, arr2[j].key);
                ASSERT_EQ(arr1[j].value, arr2[j].value);
            }
            // ASSERT_EQ(arr1, arr2);
        }

        ASSERT_EQ(ret1.wf, ret2.wf);
        ASSERT_EQ(ret1.average_length, ret2.average_length);
        ASSERT_EQ(ret1.average_frequency, ret2.average_frequency);
        ASSERT_EQ(ret1.standard_deviation, ret2.standard_deviation);
        ASSERT_EQ(ret1.val_crc, ret2.val_crc);

        // RBV Final Check
        ASSERT_EQ_FINAL(ret1.final_result_size, ret2.final_result_size);
        ASSERT_EQ_FINAL(ret1.final_results.size(), ret2.final_results.size());
        for (int i = 0; i < ret1.final_results.size(); i++) {
            // auto& [k1, v1] = ret1.final_results[i];
            // auto& [k2, v2] = ret2.final_results[i];
            // std::cout << k1 << ", " << v1 << std::endl;
            // std::cout << k2 << ", " << v2 << std::endl;
            ASSERT_EQ_FINAL(ret1.final_results[i], ret2.final_results[i]);
        }

        return true;
    }
};
}  // namespace NO_FAULT_INJECTION

using namespace NO_FAULT_INJECTION;


template<bool isMain>
auto main_fn(const char *buffer, size_t file_size, map_reduce_config config) {
    std::cout << (isMain ? "Primary" : "Replica") << " running\n";
#ifdef PROFILE_MEM
    profile::mem::start();
#endif
    Results* ret = new Results();

    // split
    std::vector<std::string_view> splits;
    splits.reserve(config.n_mappers);
    size_t split_size = file_size / config.n_mappers;
    const char *const end = buffer + file_size;
    const char *cursor = buffer;
    for (size_t i = 0; i < config.n_mappers; ++i) {
        const char *const split_start = cursor;
        const char *split_end;
        if (i == config.n_mappers - 1) {
            split_end = end;
        } else {
            split_end = buffer + split_size * (i + 1);
            if (split_end <= cursor) {
                split_end = cursor;
            }
            while (split_end < end && !std::isspace(*split_end)) {
                ++split_end;
            }
        }
        splits.emplace_back(split_start, split_end - split_start);
        assert(split_end <= end);
        cursor = split_end;
    }
    auto end_split_time = rdtsc();
    ret->splits = splits;


    // map
    auto map_results =
        __scee_run(create_result_array, config.n_mappers * config.n_reducers);
    std::vector<scee::AppThread> mapper_threads;
    mapper_threads.reserve(config.n_threads);
    for (size_t i = 0; i < config.n_threads; ++i) {
        mapper_threads.emplace_back([&, i] {
            for (size_t j = i; j < config.n_mappers; j += config.n_threads) {
                __scee_run(word_count_map_worker, splits[j], map_results, j,
                           config);
            }
        });
    }
    for (size_t i = 0; i < config.n_threads; ++i) {
        mapper_threads[i].join();
    }
    auto end_map_time = rdtsc();
    for (auto i = 0 ; i < config.n_threads; i++) {
        ret->map_results[i] = map_results.get(i).load();
    }

    // reduce
    auto reduce_results = __scee_run(create_result_array, config.n_reducers);
    std::vector<scee::AppThread> reducer_threads;
    reducer_threads.reserve(config.n_threads);
    for (size_t i = 0; i < config.n_threads; ++i) {
        reducer_threads.emplace_back([&, i] {
            for (size_t j = i; j < config.n_reducers; j += config.n_threads) {
                __scee_run(word_count_reduce_worker, map_results,
                           reduce_results, j, config);
            }
        });
    }
    for (size_t i = 0; i < config.n_threads; ++i) {
        reducer_threads[i].join();
    }
    __scee_run(destroy_result_array, map_results);
    auto end_reduce_time = rdtsc();
    for (auto i = 0 ; i < config.n_threads; i++) {
        ret->reduce_results[i] = reduce_results.get(i).load();
    }

    // sort
    auto [final_results, final_result_size] =
        __scee_run(sort_results, reduce_results, config);
    __scee_run(destroy_result_array, reduce_results);

    // print
    printf("%zu unique words\n", final_result_size);
    printf("final results:\n");

    ret->final_result_size = final_result_size;
    const auto *fr = final_results.deref_nullable();
    for (size_t i = 0; i < std::min<size_t>(final_result_size, 10); ++i) {
        printf("%8.*s: %lu\n", (int)fr[i].key.size, fr[i].key.data.deref(),
               fr[i].value);
        ret->final_results.push_back({
            std::string(fr[i].key.data.deref(), (int)fr[i].key.size),
            fr[i].value
        });
    }

#ifdef PROFILE_MEM
    profile::mem::stop();
#endif
    auto wf = __scee_run(calc_each_word_frequency, final_results, final_result_size);
    printf("wf: %f\n", wf);

    auto average_length = __scee_run(calc_average_length, final_results, final_result_size);
    printf("average length: %f\n", average_length);


    auto average_frequency = __scee_run(calc_average_frequency, final_results, final_result_size);
    printf("average frequency: %f\n", average_frequency);

    auto standard_deviation = __scee_run(calc_standard_deviation, final_results, final_result_size);
    printf("standard deviation: %f\n", standard_deviation);

    ret->wf = wf;
    ret->average_length = average_length;
    ret->average_frequency = average_frequency;
    ret->standard_deviation = standard_deviation;

    auto val_crc = __scee_run(calculate_value, &average_frequency, sizeof(double));
    printf("val_crc: %u\n", val_crc);
    ret->val_crc = val_crc;

    // return 0;
    ret->ret = 0;
    return ret;
}

int main(int argc, char **argv) {
    auto args = parse_args(argc, argv);
    auto [buffer, file_size] = load_file(args.input_file);

    // main
    std::future<Results*> fut1 = std::async(
        std::launch::async, [&](){
            return scee::main_thread(
                main_fn<true>, buffer, file_size, args.config);
        }
    );

    // rbv
    std::future<Results*> fut2 = std::async(
        std::launch::async, [&](){
            return scee::main_thread(
                main_fn<false>, buffer, file_size, args.config);
        }
    );

    auto* ret1 = fut1.get();
    auto* ret2 = fut2.get();

    post_check(*ret1, *ret2, args.config);

    free(buffer);
    return 0;
}
