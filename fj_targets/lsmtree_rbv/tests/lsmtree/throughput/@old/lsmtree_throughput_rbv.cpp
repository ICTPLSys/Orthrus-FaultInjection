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

#include "common.h"


int main_fn() {
	using namespace raw::lsmtree;
    void* lsmtree1 = new LSMTree("/dev/shm/lsmtree-rbv1");
    void* lsmtree2 = new LSMTree("/dev/shm/lsmtree-rbv2");
	std::cout << "Start LSMTree benchmark" << std::endl;

	const int ACC_CNT = 44;

	static_assert(sizeof(int64_t) == sizeof(void*));
	int64_t* sim_buf_dst = (int64_t*)malloc(sizeof(void*) * ACC_CNT * 2);
	int64_t* sim_buf_src = (int64_t*)malloc(sizeof(void*) * ACC_CNT * 2);

	auto start = microtime();
	// avg. access chain length: accCnt = 44
	//		Measured by `lsmtree_throughput_stats`
	// avg. access internal states: accCnt * sizeof(void*)
	//      ignore the Data update or malloc cost
	for (int i = 0; i < FLAGS_total_ops; i++) {
        int64_t key = all_keys[zipf_key_indices[i]];
        int64_t value = all_vals[zipf_key_indices[i]];

        auto ret1 = raw::lsmtree_set(lsmtree1, key, value);

		// Sim Internal State Copy
		// for (int j = 0; j < ACC_CNT; j++) {
		// 	sim_buf_src[j] = zipf_key_indices[i % 10000 + j];
		// }
		// memcpy(sim_buf_dst, sim_buf_src, ACC_CNT * sizeof(void*));

        auto ret2 = raw::lsmtree_set(lsmtree2, key, value);
		// scee::run2<int>(
		// 	raw::lsmtree_set,
		// 	raw::lsmtree_set,
		// 	lsmtree, key, value
		// );
	}

	auto end = microtime();
	double duration = microtime_diff(start, end);
	std::cerr << "execution time: " << duration \
        << ", throughput: " << 1.0 * FLAGS_total_ops  / (duration / 1.0E6) << "\n";

    return 0;
}

int main(int argc, char* argv[]) {
	profile::mem::init_mem("memory_status_rbv.log");
    gflags::SetUsageMessage("Usage: ./lsmtree_throughput_raw [--total-ops=<int>]");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

	init_array();
	init_rng();
	prepare_key();
	prepare_zipf_index();

    profile::start();
    scee::main_thread(main_fn);
    profile::stop();

    gflags::ShutDownCommandLineFlags();
	return 0;
}
