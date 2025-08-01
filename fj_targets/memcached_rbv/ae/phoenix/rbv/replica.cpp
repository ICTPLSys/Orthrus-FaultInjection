#include <boost/program_options.hpp>
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <utility>

#include "phoenix.hpp"
#include "scee.hpp"
#include "thread.hpp"
#include "../load_file.hpp"

#include "rbv.hpp"
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

std::pair<map_reduce_config, size_t> parse_args(int argc, char **argv) {
    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "produce help message")
        ("num-buckets,b", po::value<size_t>()->default_value(8 * 1024),
         "number of buckets")
        ("num-mappers,m", po::value<size_t>()->default_value(3 * 1024),
         "number of mappers")
        ("num-reducers,r", po::value<size_t>()->default_value(128),
         "number of reducers")
        ("num-threads,t", po::value<size_t>()->default_value(16),
         "number of threads")
        ("replica-port,pr", po::value<size_t>()->default_value(11211),
         "replica port listen to primary");
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        exit(0);
    }

    auto config = map_reduce_config{
        .n_mappers = vm["num-mappers"].as<size_t>(),
        .n_reducers = vm["num-reducers"].as<size_t>(),
        .n_buckets = vm["num-buckets"].as<size_t>(),
        .n_threads = vm["num-threads"].as<size_t>(),
        .replica_port = 22334, // not used
    };
    sprintf(config.replica_ip, "%s", "localhost"); // not used

    return std::make_pair(config, vm["replica-port"].as<size_t>());
}

int _port_;

int main_fn(const char *buffer, size_t file_size, map_reduce_config config) {
#ifdef PROFILE_MEM
    profile::mem::start();
#endif
    auto start_time = rdtsc();

    // scee::core_id = 24;
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listen_fd >= 0);

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(_port_ + 1),
        .sin_addr = {.s_addr = INADDR_ANY},
    };
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
        0) {
        close(listen_fd);
        assert("bind error" && false);
    }
    if (listen(listen_fd, 32) < 0) {
        close(listen_fd);
        assert("listen error" && false);
    }
    printf("replica listening on port %d\n", _port_ + 1);

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
        raw::create_result_array(config.n_mappers * config.n_reducers);
    std::vector<scee::AppThread> mapper_threads;
    mapper_threads.reserve(config.n_mappers);

    {
        int clock_fd = accept(listen_fd, nullptr, nullptr);
        assert(clock_fd >= 0);
        fd_reader clock_reader(clock_fd);
        size_t len = clock_reader.read_packet('\x01');
        assert(len > 0);
        long long client_us = atoll(std::string(clock_reader.packet, len - 1).c_str());
        long long server_us = profile::get_us_abs();
        write_all(clock_fd, std::to_string(server_us) + "\x01");
    }

    int ack_fd = accept(listen_fd, nullptr, nullptr);
    assert(ack_fd >= 0);

#ifdef PROFILE
    profile::start();
#endif
    std::atomic_int active_threads = 0;
    for (size_t k = 0; k < config.n_mappers; ++k) {
        int client_fd = accept(listen_fd, nullptr, nullptr);
        assert(client_fd >= 0);
        mapper_threads.emplace_back([&, client_fd] {
            fd_reader reader(client_fd);
            size_t len = reader.read_packet('\x01');
            assert(len > 0);
            size_t i = atoi(std::string(reader.packet, len - 1).c_str());
            assert(i < config.n_mappers);
            while (active_threads.load() >= config.n_threads * 4) my_usleep(rand() % 10000);
            active_threads.fetch_add(1);
            write_all(client_fd, "ACK\x01");
            len = reader.read_packet('\x01');
            assert(len > 0);
            long long start_my_us = profile::get_us_abs();
            long long start_us = atoll(std::string(reader.packet, 20).c_str());
            std::string s = std::string(reader.packet + 20, len - 21);
            rbv::hasher.deserialize(s);
            raw::word_count_map_worker(splits[i], map_results, i, config);
            rbv::hasher.finalize();
            long long end_us = profile::get_us_abs();
            active_threads.fetch_sub(1);
            close(client_fd);
#ifdef PROFILE
            profile::record_validation_latency(end_us - start_us);
            profile::record_validation_cpu_time((end_us - start_my_us) * kCpuMhzNorm, 1);
#endif
        });
    }
    for (size_t i = 0; i < config.n_mappers; ++i) {
        mapper_threads[i].join();
    }
    auto end_map_time = rdtsc();

    write_all(ack_fd, "ACK\x01");

#ifdef PROFILE
    profile::stop();
#endif

    // reduce
    auto reduce_results = raw::create_result_array(config.n_reducers);
    std::vector<scee::AppThread> reducer_threads;
    reducer_threads.reserve(config.n_reducers);
    for (size_t k = 0; k < config.n_reducers; ++k) {
        int client_fd = accept(listen_fd, nullptr, nullptr);
        assert(client_fd >= 0);
        reducer_threads.emplace_back([&, client_fd] {
            fd_reader reader(client_fd);
            size_t len = reader.read_packet('\x02');
            assert(len > 0);
            size_t i = atoi(std::string(reader.packet, len - 1).c_str());
            assert(i < config.n_reducers);
            while (active_threads.load() >= config.n_threads * 4) my_usleep(rand() % 10000);
            active_threads.fetch_add(1);
            write_all(client_fd, "ACK\x02");
            len = reader.read_packet('\x02');
            assert(len > 0);
            long long start_my_us = profile::get_us_abs();
            long long start_us = atoll(std::string(reader.packet, 20).c_str());
            std::string s = std::string(reader.packet + 20, len - 21);
            rbv::hasher.deserialize(s);
            raw::word_count_reduce_worker(map_results, reduce_results, i, config);
            rbv::hasher.finalize();
            long long end_us = profile::get_us_abs();
            active_threads.fetch_sub(1);
#ifdef PROFILE
            profile::record_validation_latency(end_us - start_us);
            profile::record_validation_cpu_time((end_us - start_my_us) * kCpuMhzNorm, 1);
#endif
            write_all(client_fd, "ACK\x02");
        });
    }
    for (size_t i = 0; i < config.n_reducers; ++i) {
        reducer_threads[i].join();
    }
    raw::destroy_result_array(map_results);
    auto end_reduce_time = rdtsc();

    write_all(ack_fd, "ACK\x01");

    // sort
    auto [final_results, final_result_size] =
        raw::sort_results(reduce_results, config);
    raw::destroy_result_array(reduce_results);

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
        printf("%8.*s: %lu\n", (int)fr[i].key.size, fr[i].key.data.deref(),
               fr[i].value);
    }
    printf("\n");

#ifdef PROFILE_MEM
    profile::mem::stop();
#endif
    return 0;
}

int main(int argc, char **argv) {
    auto args = parse_args(argc, argv);
    _port_ = args.second;
    auto [buffer, file_size] = load_file(_port_);
    int ret = scee::main_thread(main_fn, buffer, file_size, args.first);
    free(buffer);
    return ret;
}
