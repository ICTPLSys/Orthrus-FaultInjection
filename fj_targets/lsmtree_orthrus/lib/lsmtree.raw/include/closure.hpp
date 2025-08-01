int lsmtree_set(void *lsm, int64_t k, int64_t v);
int64_t lsmtree_get(void *lsm, int64_t k);
int lsmtree_del(void *lsm, int64_t k);
const app::lsmtree::InternalStates& lsmtree_get_internal_states(void* lsm);

// using namespace lsmtree;
// Retcode lsmtree_set(LSMTree *lsm, KeyT k, ValueT v);
// Retcode lsmtree_get(LSMTree *lsm, KeyT k, ValueT *v);
// Retcode lsmtree_del(LSMTree *lsm, KeyT k);
