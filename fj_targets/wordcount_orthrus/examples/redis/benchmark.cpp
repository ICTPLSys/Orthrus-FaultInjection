#include <assert.h>
#include <stdio.h>

#include <map>
#include <random>
#include <string>
#include <utility>

#include "context.hpp"
#include "ctltypes.hpp"
#include "custom_stl.hpp"
#include "log.hpp"
#include "namespace.hpp"
#include "ptr.hpp"
#include "scee.hpp"
#include "thread.hpp"

namespace raw {
#include "closure.hpp"
}  // namespace raw
namespace app {
#include "closure.hpp"
}  // namespace app
namespace validator {
#include "closure.hpp"
}  // namespace validator

using namespace raw;

std::mt19937 rng(1234567);

std::map<std::string, std::string> dict;

std::pair<Key, Val> mkentry(const int alphaSize, bool inside = false) {
    Key key;
    Val val;
    while (true) {
        for (size_t i = 0; i < KEY_LEN; ++i)
            key.ch[i] = rng() % alphaSize + 'A';
        if (inside) {
            assert(dict.size() > 0);
            auto it = dict.lower_bound(key.to_string());
            if (it == dict.end()) it = dict.begin();
            for (size_t i = 0; i < KEY_LEN; ++i) key.ch[i] = it->first[i];
            break;
        } else if (!dict.count(key.to_string()))
            break;
    }
    for (size_t i = 0; i < VAL_LEN; ++i) val.ch[i] = rng() % alphaSize + 'a';
    return std::make_pair(key, val);
}

// constexpr int TestCnt = 1, MaxCap = 8, SizeIn = 1000, Nq = 1000, Alphabet =
// 2;
constexpr int TestCnt = 10, MaxCap = 1024, SizeIn = 1E4, Nq = 1E5, Alphabet = 4;

int main_fn() {
    rng.seed(1);
    for (int T = 1; T <= TestCnt; ++T) {
        hashmap_t hmap_unsafe = hashmap_t::make(MaxCap);
        ptr_t<hashmap_t> *hm_safe = ptr_t<hashmap_t>::create(hmap_unsafe);
        for (int q = -SizeIn; q < Nq; ++q) {
            int op = (q < 0 ? 0 : rng() % 10);
            std::pair<Key, Val> entry = mkentry(Alphabet, op % 2);
            int verdict;
            if (op == 0 || op == 1) {
                using HashmapSetType = scee::ptr_t<scee::imm_string_t>
                    *(*)(scee::ptr_t<hashmap_t> *, Key, Val);
                HashmapSetType app_fn =
                    reinterpret_cast<HashmapSetType>(app::hashmap_set);
                HashmapSetType val_fn =
                    reinterpret_cast<HashmapSetType>(validator::hashmap_set);
                const imm_string_t *out_ptr =
                    scee::run2(app_fn, val_fn, hm_safe, entry.first,
                               entry.second)
                        ->load();
                std::string out(out_ptr->v, out_ptr->length());
                std::string ans =
                    op % 2 ? kRetVals[kStored] : kRetVals[kCreated];
                out.pop_back();
                out.pop_back();
                ans.pop_back();
                ans.pop_back();
                if (out != ans) {
                    fprintf(stderr, "Set error: expect %s, found %s\n",
                            ans.c_str(), out.c_str());
                    exit(1);
                }
                dict[entry.first.to_string()] = entry.second.to_string();
            } else if (op == 8 || op == 9) {
                using HashmapDelType = scee::ptr_t<scee::imm_string_t>
                    *(*)(scee::ptr_t<hashmap_t> *, Key);
                HashmapDelType app_fn =
                    reinterpret_cast<HashmapDelType>(app::hashmap_del);
                HashmapDelType val_fn =
                    reinterpret_cast<HashmapDelType>(validator::hashmap_del);
                const imm_string_t *out_ptr =
                    scee::run2(app_fn, val_fn, hm_safe, entry.first)->load();
                std::string out(out_ptr->v, out_ptr->length());
                std::string ans =
                    op % 2 ? kRetVals[kDeleted] : kRetVals[kNotFound];
                out.pop_back();
                out.pop_back();
                ans.pop_back();
                ans.pop_back();
                if (out != ans) {
                    fprintf(stderr, "Del error: expect %s, found %s\n",
                            ans.c_str(), out.c_str());
                    exit(1);
                }
                dict.erase(entry.first.to_string());
            } else {
                using HashmapGetType = scee::ptr_t<scee::imm_string_t>
                    *(*)(scee::ptr_t<hashmap_t> *, Key);
                HashmapGetType app_fn =
                    reinterpret_cast<HashmapGetType>(app::hashmap_get);
                HashmapGetType val_fn =
                    reinterpret_cast<HashmapGetType>(validator::hashmap_get);
                const imm_string_t *out_ptr =
                    scee::run2(app_fn, val_fn, hm_safe, entry.first)->load();
                std::string out(out_ptr->v, out_ptr->length());
                std::string ans = op % 2 ? kRetVals[kValue] +
                                               dict[entry.first.to_string()] +
                                               kCrlf
                                         : kRetVals[kEnd];
                out.pop_back();
                out.pop_back();
                ans.pop_back();
                ans.pop_back();
                if (out != ans) {
                    fprintf(stderr, "Get error: expect %s, found %s\n",
                            ans.c_str(), out.c_str());
                    exit(1);
                }
            }
        }
        dict.clear();
    }
    fprintf(stderr, "Test passed!!!\n");
    return 0;
}

int main() { return scee::main_thread(main_fn); }