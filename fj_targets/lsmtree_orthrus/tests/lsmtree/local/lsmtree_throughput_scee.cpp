#include <filesystem>
#include <iostream>
// #include <format>
#include <cstdio>
#include <thread>

#include "utils.hpp"
#include <gflags/gflags.h>

#include "context.hpp"
#include "ctltypes.hpp"
#include "custom_stl.hpp"
#include "log.hpp"
#include "lsmtree-closure.hpp"
#include "lsmtree.hpp"
#include "namespace.hpp"
#include "ptr.hpp"
#include "scee.hpp"
#include "thread.hpp"

#include "profile-mem.hpp"

#include "common.h"

int main_fn() {
	using namespace raw::lsmtree;
    void* lsmtree = new LSMTree("/dev/shm/lsmtree-scee");
	std::cout << "Start LSMTree benchmark" << std::endl;

	auto start = microtime();
	for (int i = 0; i < kNumOpsPerThread; i++) {
			int64_t key = all_keys[zipf_key_indices[i]];
			int64_t value = all_vals[zipf_key_indices[i]];

            // fprintf(stderr, "%d/%d\n", i, kNumOpsPerThread);
            scee::run2<int>(
                app::lsmtree_set,
                validator::lsmtree_set,
                lsmtree, key, value
            );
	}

	auto end = microtime();
	double duration = microtime_diff(start, end);
	std::cerr << "execution time: " << duration \
        << ", throughput: " << 1.0 * FLAGS_total_ops  / (duration / 1.0E6) << "\n";

	// sleep(2);
	// {
	// 	using namespace scee;
	// 	validation_latency_lock.lock();
	// 	std::sort(validation_latency.begin(), validation_latency.end());
	// 	auto avg = std::accumulate(validation_latency.cbegin(), validation_latency.cend(), 0.0) /
	// validation_latency.size(); 	auto p90 = validation_latency[validation_latency.size() * 90 /
	// 100]; 	auto p95 = validation_latency[validation_latency.size() * 95 / 100]; 	auto p99 =
	// validation_latency[validation_latency.size() * 99 / 100]; validation_latency_lock.unlock();
	// 	std::cout << std::format("avg: {}, p90: {}, p95: {}, p99: {}\n", avg, p90, p95, p99);
	// }
    return 0;
}

int main(int argc, char* argv[]) {
	profile::mem::init_mem("memory_status_scee.log");
    gflags::SetUsageMessage("Usage: ./lsmtree_throughput_scee [--total-ops=<int>]");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

	init_array();
	init_rng();
	prepare_key();
	prepare_zipf_index();

	profile::mem::start();
    scee::main_thread(main_fn);
	profile::mem::stop();

    gflags::ShutDownCommandLineFlags();
	return 0;
}
