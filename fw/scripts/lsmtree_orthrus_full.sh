#! /bin/bash

source table2_env_runtime.sh

rm -rf tests/lsmtree_comp_orthrus_full/
rm -rf tests/lsmtree_cons_orthrus_full/

run_with_timeout 28800 LOG_LEVEL=INFO python test_ng.py \
    --tag lsmtree_comp_orthrus_full \
    --debug \
    --test-type lsmtree \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/lsmtree_orthrus/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions both \
    --ft-type orthrus_full \
    --output output/lsmtree_comp_orthrus_full.json > logs/lsmtree_comp_orthrus_full.log 2>&1

wait

run_with_timeout 28800 LOG_LEVEL=INFO python test_ng.py \
    --tag lsmtree_cons_orthrus_full \
    --debug \
    --test-type lsmtree \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/lsmtree_orthrus/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category consistency \
    --fj-functions both \
    --ft-type orthrus_full \
    --output output/lsmtree_cons_orthrus_full.json > logs/lsmtree_cons_orthrus_full.log 2>&1

python3 parse.py tests/lsmtree_comp_orthrus_full/cache/func_fj_infos.json  output/lsmtree_comp_orthrus_full.json lsmtree_orthrus_computational
python3 parse.py tests/lsmtree_cons_orthrus_full/cache/func_fj_infos.json  output/lsmtree_cons_orthrus_full.json lsmtree_orthrus_consistency