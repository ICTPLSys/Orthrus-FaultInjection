#! /bin/bash

rm -rf tests/lsmtree_comp_rbv_fastcheck/
rm -rf tests/lsmtree_cons_rbv_fastcheck/

LOG_LEVEL=INFO python test_ng.py \
    --tag lsmtree_comp_rbv_fastcheck \
    --debug \
    --test-type lsmtree_rbv \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/lsmtree_rbv/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions onlyapp \
    --ft-type rbv_fastcheck \
    --test_mode lite \
    --output output/lsmtree_comp_rbv_fastcheck.json > logs/lsmtree_comp_rbv_fastcheck.log 2>&1

wait

# LOG_LEVEL=INFO python test_ng.py \
#     --tag lsmtree_cons_rbv_fastcheck \
#     --debug \
#     --test-type lsmtree_rbv \
#     --temp-dir ./tests/ \
#     --scee-dir ${CWD}/fj_targets/lsmtree_rbv/ \
#     --llvm-dir ${LLVM_INSTALL_PATH} \
#     --t-build 40 \
#     --t-test 40 \
#     --category consistency \
#     --fj-functions onlyapp \
#     --ft-type rbv_fastcheck \
#     --test_mode lite \
#     --output output/lsmtree_cons_rbv_fastcheck.json > logs/lsmtree_cons_rbv_fastcheck.log 2>&1

python3 parse.py tests/lsmtree_comp_rbv_fastcheck/cache/func_fj_infos.json  output/lsmtree_comp_rbv_fastcheck.json lsmtree_rbv_computational
# python3 parse.py tests/lsmtree_cons_rbv_fastcheck/cache/func_fj_infos.json  output/lsmtree_cons_rbv_fastcheck.json lsmtree_rbv_consistency