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
        ("replica-ip,ipr", po::value<std::string>()->default_value("localhost"),
         "replica ip listen to primary")
        ("replica-port,pr", po::value<size_t>()->default_value(11211),
         "replica port listen to primary")
        ("primary-port,pp", po::value<size_t>()->default_value(12221),
         "primary port listen to loader");
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
        .replica_port = vm["replica-port"].as<size_t>(),
    };
    sprintf(config.replica_ip, "%s", vm["replica-ip"].as<std::string>().c_str());

    return std::make_pair(config, vm["primary-port"].as<size_t>());
}

int main_fn(const char *buffer, size_t file_size, map_reduce_config config) {
#ifdef PROFILE_MEM
    profile::mem::start();
#endif
    my_usleep(1000000); // wait until replica server to start

    auto start_time = rdtsc();
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

    long long time_diff = 0;
    {
        int clock_fd = connect_server(config.replica_ip, config.replica_port + 1);
        fd_reader clock_reader(clock_fd);
        long long client_us = profile::get_us_abs();
        write_all(clock_fd, std::to_string(client_us) + "\x01");
        size_t len = clock_reader.read_packet('\x01');
        assert(len > 0);
        long long server_us = atoll(std::string(clock_reader.packet, len - 1).c_str());
        long long client_us_2 = profile::get_us_abs();
        // time_diff = server_us - (client_us_2 + client_us) / 2;
        time_diff = server_us - client_us_2;
    }

    int ack_fd = connect_server(config.replica_ip, config.replica_port + 1);
    assert(ack_fd >= 0);
    fd_reader ack_reader(ack_fd);

    // map
    auto map_results =
        raw::create_result_array(config.n_mappers * config.n_reducers);
    std::vector<scee::AppThread> mapper_threads;
    mapper_threads.reserve(config.n_threads);
    for (size_t i = 0; i < config.n_threads; ++i) {
        mapper_threads.emplace_back([&, i] {
            for (size_t j = i; j < config.n_mappers; j += config.n_threads) {
                int fd = connect_server(config.replica_ip, config.replica_port + 1);
                assert(fd >= 0);
                fd_reader reader(fd);
                write_all(fd, std::to_string(j) + "\x01");
                reader.read_packet('\x01');
                long long start_us = profile::get_us_abs() + time_diff;
                raw::word_count_map_worker(splits[j], map_results, j, config);
                std::string s = rbv::hasher.finalize();
                write_all(fd, rbv::toString20(start_us) + s + "\x01");
                close(fd);
            }
        });
    }
    for (size_t i = 0; i < config.n_threads; ++i) {
        mapper_threads[i].join();
    }
    auto end_map_time = rdtsc();

    ack_reader.read_packet('\x01');

    // reduce
    auto reduce_results = raw::create_result_array(config.n_reducers);
    std::vector<scee::AppThread> reducer_threads;
    reducer_threads.reserve(config.n_threads);
    for (size_t i = 0; i < config.n_threads; ++i) {
        reducer_threads.emplace_back([&, i] {
            for (size_t j = i; j < config.n_reducers; j += config.n_threads) {
                int fd = connect_server(config.replica_ip, config.replica_port + 1);
                assert(fd >= 0);
                fd_reader reader(fd);
                write_all(fd, std::to_string(j) + "\x02");
                reader.read_packet('\x02');
                long long start_us = profile::get_us_abs() + time_diff;
                raw::word_count_reduce_worker(map_results, reduce_results, j, config);
                std::string s = rbv::hasher.finalize();
                write_all(fd, rbv::toString20(start_us) + s + "\x02");
                close(fd);
            }
        });
    }
    for (size_t i = 0; i < config.n_threads; ++i) {
        reducer_threads[i].join();
    }
    raw::destroy_result_array(map_results);
    auto end_reduce_time = rdtsc();

    ack_reader.read_packet('\x01');

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
    replica_fd = connect_server(args.first.replica_ip, args.first.replica_port);
    auto [buffer, file_size] = load_file(args.second);
    close(replica_fd);
    int ret = scee::main_thread(main_fn, buffer, file_size, args.first);
    free(buffer);
    return ret;
}
