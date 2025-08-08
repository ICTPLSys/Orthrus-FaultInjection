#! /bin/bash

rm -rf tests/lsmtree_comp_orthrus_fastcheck/
rm -rf tests/lsmtree_cons_orthrus_fastcheck/

LOG_LEVEL=INFO python test_ng.py \
    --tag lsmtree_comp_orthrus_fastcheck \
    --debug \
    --test-type lsmtree \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/lsmtree_orthrus/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions both \
    --ft-type orthrus_fastcheck \
    --test_mode lite \
    --output output/lsmtree_comp_orthrus_fastcheck.json > logs/lsmtree_comp_orthrus_fastcheck.log 2>&1

wait

LOG_LEVEL=INFO python test_ng.py \
    --tag lsmtree_cons_orthrus_fastcheck \
    --debug \
    --test-type lsmtree \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/lsmtree_orthrus/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category consistency \
    --fj-functions both \
    --ft-type orthrus_fastcheck \
    --test_mode lite \
    --output output/lsmtree_cons_orthrus_fastcheck.json > logs/lsmtree_cons_orthrus_fastcheck.log 2>&1

python3 parse.py tests/lsmtree_comp_orthrus_fastcheck/cache/func_fj_infos.json  output/lsmtree_comp_orthrus_fastcheck.json lsmtree_orthrus_computational
python3 parse.py tests/lsmtree_cons_orthrus_fastcheck/cache/func_fj_infos.json  output/lsmtree_cons_orthrus_fastcheck.json lsmtree_orthrus_consistency