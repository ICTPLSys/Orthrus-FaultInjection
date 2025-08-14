#! /bin/bash

rm -rf tests/lsmtree_comp_rbv_full/
rm -rf tests/lsmtree_cons_rbv_full/

rm -rf tests/lsmtree_comp_rbv_full/build/
rm -rf tests/lsmtree_cons_rbv_full/build/

source ../table2_env_runtime.sh

run_with_timeout 28800 bash -c "LOG_LEVEL=INFO python test_ng.py \
    --tag lsmtree_comp_rbv_full \
    --debug \
    --test-type lsmtree_rbv \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/lsmtree_rbv/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions onlyapp \
    --ft-type rbv_full \
    --output output/lsmtree_comp_rbv_full.json > logs/lsmtree_comp_rbv_full.log 2>&1"

wait

run_with_timeout 28800 bash -c "LOG_LEVEL=INFO python test_ng.py \
    --tag lsmtree_cons_rbv_full \
    --debug \
    --test-type lsmtree_rbv \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/lsmtree_rbv/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category consistency \
    --fj-functions onlyapp \
    --ft-type rbv_full \
    --output output/lsmtree_cons_rbv_full.json > logs/lsmtree_cons_rbv_full.log 2>&1"

python3 parse.py tests/lsmtree_comp_rbv_full/cache/func_fj_infos.json  output/lsmtree_comp_rbv_full.json lsmtree_rbv_computational
python3 parse.py tests/lsmtree_cons_rbv_full/cache/func_fj_infos.json  output/lsmtree_cons_rbv_full.json lsmtree_rbv_consistency
