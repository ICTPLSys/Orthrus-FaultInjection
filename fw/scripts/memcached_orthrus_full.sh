#! /bin/bash

rm -rf tests/hashmap_comp_orthrus_full/
rm -rf tests/hashmap_cons_orthrus_full/

LOG_LEVEL=INFO python test_ng.py \
    --tag hashmap_comp_orthrus_full \
    --debug \
    --test-type hashmap \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/memcached_orthrus/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions both \
    --ft-type orthrus_full \
    --output output/hashmap_comp_orthrus_full.json > logs/hashmap_comp_orthrus_full.log 2>&1

wait

LOG_LEVEL=INFO python test_ng.py \
    --tag hashmap_cons_orthrus_full \
    --debug \
    --test-type hashmap \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/memcached_orthrus/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category consistency \
    --fj-functions both \
    --ft-type orthrus_full \
    --output output/hashmap_cons_orthrus_full.json > logs/hashmap_cons_orthrus_full.log 2>&1

python3 parse.py tests/hashmap_comp_orthrus_full/cache/func_fj_infos.json  output/hashmap_comp_orthrus_full.json hashmap_orthrus_computational
python3 parse.py tests/hashmap_cons_orthrus_full/cache/func_fj_infos.json  output/hashmap_cons_orthrus_full.json hashmap_orthrus_consistency