#! /bin/bash

rm -rf tests/hashmap_comp_rbv_full/
rm -rf tests/hashmap_cons_rbv_full/

rm -rf tests/hashmap_comp_rbv_full/build/
rm -rf tests/hashmap_cons_rbv_full/build/

source ../table2_env_runtime.sh

run_with_timeout 28800 bash -c "LOG_LEVEL=INFO python test_ng.py \
    --tag hashmap_comp_rbv_full \
    --debug \
    --test-type hashmap_rbv \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/memcached_rbv/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions onlyapp \
    --ft-type rbv_full \
    --output output/hashmap_comp_rbv_full.json > logs/hashmap_comp_rbv_full.log 2>&1"

wait

run_with_timeout 28800 bash -c "LOG_LEVEL=INFO python test_ng.py \
    --tag hashmap_cons_rbv_full \
    --debug \
    --test-type hashmap_rbv \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/memcached_rbv/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category consistency \
    --fj-functions onlyapp \
    --ft-type rbv_full \
    --output output/hashmap_cons_rbv_full.json > logs/hashmap_cons_rbv_full.log 2>&1"

python3 parse.py tests/hashmap_comp_rbv_full/cache/func_fj_infos.json  output/hashmap_comp_rbv_full.json hashmap_rbv_computational
python3 parse.py tests/hashmap_cons_rbv_full/cache/func_fj_infos.json  output/hashmap_cons_rbv_full.json hashmap_rbv_consistency