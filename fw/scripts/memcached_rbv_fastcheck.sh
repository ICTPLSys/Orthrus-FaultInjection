#! /bin/bash

rm -rf tests/hashmap_comp_rbv_fastcheck/
rm -rf tests/hashmap_cons_rbv_fastcheck/

LOG_LEVEL=INFO python test_ng.py \
    --tag hashmap_comp_rbv_fastcheck \
    --debug \
    --test-type hashmap_rbv \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/memcached_rbv/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions onlyapp \
    --ft-type rbv_fastcheck \
    --test_mode lite \
    --output output/hashmap_comp_rbv_fastcheck.json > logs/hashmap_comp_rbv_fastcheck.log 2>&1

wait

LOG_LEVEL=INFO python test_ng.py \
    --tag hashmap_cons_rbv_fastcheck \
    --debug \
    --test-type hashmap_rbv \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/memcached_rbv/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category consistency \
    --fj-functions onlyapp \
    --ft-type rbv_fastcheck \
    --test_mode lite \
    --output output/hashmap_cons_rbv_fastcheck.json > logs/hashmap_cons_rbv_fastcheck.log 2>&1

python3 parse.py tests/hashmap_comp_rbv_fastcheck/cache/func_fj_infos.json  output/hashmap_comp_rbv_fastcheck.json hashmap_rbv_computational
python3 parse.py tests/hashmap_cons_rbv_fastcheck/cache/func_fj_infos.json  output/hashmap_cons_rbv_fastcheck.json hashmap_rbv_consistency