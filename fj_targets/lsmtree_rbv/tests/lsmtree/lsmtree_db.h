#pragma once

#include <filesystem>

#include "consts.h"
#include "db.h"
#include "lsmtree.h"

namespace lt = scee::lsmtree;
namespace fs = std::filesystem;

template <typename K = lt::KeyT, typename V = lt::ValueT>
class LSMTreeDatabase : public Database<K, V> {
  public:
	explicit LSMTreeDatabase(const fs::path &db_path) {
		if (fs::exists(db_path)) {
			std::cerr << "Database already exists. Deleting it." << std::endl;
			fs::remove_all(db_path);
		}
		lsm = new lt::LSMTree(db_path);
	}

	int Read(K key, V *value) override {
		// fprintf(stderr, "Read: Key=%ld\n", key);
		auto ret = lsm->Get(key, value);
		// fprintf(stderr, "Read: Key=%ld, value=%ld\n", key, *value);
		return ret == scee::lsmtree::Retcode::Success ? 0 : -1;
	};

	int Scan(K key, int len, std::vector<K> &keys, std::vector<V> &vals) override {
		abort();
		return 0;
	}

	int Insert(K key, V value) override {
		// fprintf(stderr, "Insert: Key=%ld, Value=%lu\n", key, value);
		auto ret = lsm->Set(key, value);
		return ret == scee::lsmtree::Retcode::Success ? 0 : -1;
	}

	int Update(K key, V value) override {
		// fprintf(stderr, "Update: Key=%ld, Value=%lu\n", key, value);
		return this->Insert(key, value);
	}

	int BulkLoad(std::vector<K> &keys, std::vector<V> &vals) override {
		int tot = keys.size();
		auto start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < tot; i++) {
			if (i > 100000 && i % 100000 == 0) {
				auto end = std::chrono::high_resolution_clock::now();
				auto spd =
					i * 1.0 / std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
				// std::cout << std::format(
				// 	"BulkLoad {}/{}, reminds: {}secs\n", i, tot, (tot - i) / spd);
				std::cout << "BulkLoad " << i << "/" << tot ", reminds: " << (tot-i)/spd << "secs\n";
			}
			auto key = keys[i];
			auto value = vals[i];
			this->Insert(key, value);
		}

		return 0;
	}

  public:
	lt::LSMTree *lsm;
};
