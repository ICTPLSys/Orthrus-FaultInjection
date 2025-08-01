#include "context.hpp"
#include "namespace.hpp"

namespace scee {
using namespace NAMESPACE;

/**
 * OCCControl is the optimistic concurrency control structure.
 * LOCK: this node is blocked to writes;
 * INSERT: we are inserting to this node, maximum fanout-1 times;
 * SPLIT: we are splitting this node, maximum 1 time;
 * DELETED: this node have been removed from main tree;
 * INSERTCNT: count the number of times we have inserted.
 */
struct OCCControl {
    enum {
        LOCK = 1ULL,
        INSERT = LOCK << 1,
        SPLIT = INSERT << 1,
        DELETED = SPLIT << 1,
        INSERTCNT = DELETED << 1,
    };
    constexpr static uint64_t WRITING = INSERT | SPLIT;
    std::atomic<uint64_t> ver;
    explicit OCCControl(uint64_t version);
    uint64_t stable_version();
    uint64_t load();
    void lock();
    void unlock();
    void do_insert();
    void done_insert();
    void do_split();
    void done_create();
    void done_split_and_delete();
    static OCCControl *create(uint64_t version);
    void destroy();
};

FORCE_INLINE OCCControl::OCCControl(uint64_t version) : ver(version) {}

FORCE_INLINE void OCCControl::unlock() {
    if (!is_validator()) ver ^= LOCK;
}

FORCE_INLINE void OCCControl::do_insert() {
    if (!is_validator()) ver |= INSERT;
}

FORCE_INLINE void OCCControl::done_insert() {
    if (!is_validator()) {
        ver += INSERTCNT;
        ver ^= INSERT;
    }
}

FORCE_INLINE void OCCControl::do_split() {
    if (!is_validator()) ver |= SPLIT;
}

FORCE_INLINE void OCCControl::done_create() {
    if (!is_validator()) {
        ver += INSERTCNT;
        ver ^= SPLIT;
    }
}

FORCE_INLINE void OCCControl::done_split_and_delete() {
    if (!is_validator()) {
        ver += INSERTCNT;
        ver |= DELETED;
        ver ^= SPLIT;
    }
}

FORCE_INLINE OCCControl *OCCControl::create(uint64_t version) {
    auto *occ = (OCCControl *)alloc_obj(sizeof(OCCControl));
    if (!is_validator()) {
        new (occ) OCCControl(version);
    }
    return occ;
}

FORCE_INLINE void OCCControl::destroy() {
    if (!is_validator()) {
        free_immutable(this);
    }
}

}  // namespace scee
