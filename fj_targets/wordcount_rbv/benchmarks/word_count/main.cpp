#include <boost/program_options.hpp>
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <utility>

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
        ("input,i", po::value<std::string>(), "input file")
        ("num-buckets,b", po::value<size_t>()->default_value(8 * 1024),
         "number of buckets")
        ("num-mappers,m", po::value<size_t>()->default_value(1024),
         "number of mappers")
        ("num-reducers,r", po::value<size_t>()->default_value(256),
         "number of reducers")
        ("num-threads,t", po::value<size_t>()->default_value(8),
         "number of threads");
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        exit(0);
    }

    if (!vm.count("input")) {
        std::cerr << "Error: input file is required" << std::endl;
        std::cout << desc << std::endl;
        exit(1);
    }

    return Args{vm["input"].as<std::string>(),
                {
                    .n_mappers = vm["num-mappers"].as<size_t>(),
                    .n_reducers = vm["num-reducers"].as<size_t>(),
                    .n_buckets = vm["num-buckets"].as<size_t>(),
                    .n_threads = vm["num-threads"].as<size_t>(),
                }};
}

// TODO(liquanxi): should we make this a closure?
std::pair<char *, size_t> load_file(const std::string &file_path) {
    FILE *file = fopen(file_path.c_str(), "r");
    if (!file) {
        std::cerr << "Error: failed to open input file" << std::endl;
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    std::cout << "Loading file, size: " << file_size << " bytes" << std::endl;

    char *buffer = (char *)malloc(file_size);
    fread(buffer, 1, file_size, file);
    fclose(file);
    std::cout << "File loaded" << std::endl;

    return {buffer, file_size};
}

int main_fn(const char *buffer, size_t file_size, map_reduce_config config) {
    auto start_time = rdtsc();
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

    // sort
    auto [final_results, final_result_size] =
        __scee_run(sort_results, reduce_results, config);
    __scee_run(destroy_result_array, reduce_results);

    auto end_time = rdtsc();
    std::cout << std::setw(16)
              << "Time taken: " << microsecond(start_time, end_time) / 1000
              << " ms" << std::endl;
    std::cout << std::setw(16)
              << "Split: " << microsecond(start_time, end_split_time) / 1000
              << " ms" << std::endl;
    std::cout << std::setw(16)
              << "Map: " << microsecond(end_split_time, end_map_time) / 1000
              << " ms" << std::endl;
    std::cout << std::setw(16)
              << "Reduce: " << microsecond(end_map_time, end_reduce_time) / 1000
              << " ms" << std::endl;
    std::cout << std::setw(16)
              << "Sort: " << microsecond(end_reduce_time, end_time) / 1000
              << " ms" << std::endl;

    // print
    printf("%zu unique words\n", final_result_size);
    printf("final results:\n");
    const auto *fr = final_results.deref_nullable();
    for (size_t i = 0; i < std::min<size_t>(final_result_size, 10); ++i) {
        printf("%8.*s: %lu\t", (int)fr[i].key.size, fr[i].key.data.deref(),
               fr[i].value);
    }
    printf("\n");

    return 0;
}

int main(int argc, char **argv) {
    auto args = parse_args(argc, argv);
    auto start_load_time = rdtsc();
    auto [buffer, file_size] = load_file(args.input_file);
    auto end_load_time = rdtsc();
    printf("Load time: %zu ms\n",
           microsecond(start_load_time, end_load_time) / 1000);
    int ret = scee::main_thread(main_fn, buffer, file_size, args.config);
    free(buffer);
    return ret;
}
