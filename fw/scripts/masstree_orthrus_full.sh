#! /bin/bash

rm -rf tests/masstree_comp_orthrus_full/
rm -rf tests/masstree_cons_orthrus_full/

rm -rf tests/masstree_comp_orthrus_full/build/
rm -rf tests/masstree_cons_orthrus_full/build/

source ../table2_env_runtime.sh

run_with_timeout 28800 bash -c "LOG_LEVEL=INFO python test_ng.py \
    --tag masstree_comp_orthrus_full \
    --debug \
    --test-type masstree \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/masstree_orthrus/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions both \
    --ft-type orthrus_full \
    --output output/masstree_comp_orthrus_full.json > logs/masstree_comp_orthrus_full.log 2>&1"

wait

run_with_timeout 28800 bash -c "LOG_LEVEL=INFO python test_ng.py \
    --tag masstree_cons_orthrus_full \
    --debug \
    --test-type masstree \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/masstree_orthrus/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category consistency \
    --fj-functions both \
    --ft-type orthrus_full \
    --output output/masstree_cons_orthrus_full.json > logs/masstree_cons_orthrus_full.log 2>&1"

python3 parse.py tests/masstree_comp_orthrus_full/cache/func_fj_infos.json  output/masstree_comp_orthrus_full.json masstree_orthrus_computational
python3 parse.py tests/masstree_cons_orthrus_full/cache/func_fj_infos.json  output/masstree_cons_orthrus_full.json masstree_orthrus_consistency
