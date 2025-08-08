#! /bin/bash

rm -rf tests/masstree_comp_rbv_fastcheck/
rm -rf tests/masstree_cons_rbv_fastcheck/

LOG_LEVEL=INFO python test_ng.py \
    --tag masstree_comp_rbv_fastcheck \
    --debug \
    --test-type masstree \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/masstree_rbv/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions onlyapp \
    --ft-type rbv_fastcheck \
    --test_mode lite \
    --output output/masstree_comp_rbv_fastcheck.json > logs/masstree_comp_rbv_fastcheck.log 2>&1

wait 

LOG_LEVEL=INFO python test_ng.py \
    --tag masstree_cons_rbv_fastcheck \
    --debug \
    --test-type masstree \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/masstree_rbv/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category consistency \
    --fj-functions onlyapp \
    --ft-type rbv_fastcheck \
    --test_mode lite \
    --output output/masstree_cons_rbv_fastcheck.json > logs/masstree_cons_rbv_fastcheck.log 2>&1

python3 parse.py tests/masstree_comp_rbv_fastcheck/cache/func_fj_infos.json  output/masstree_comp_rbv_fastcheck.json masstree_rbv_computational
python3 parse.py tests/masstree_cons_rbv_fastcheck/cache/func_fj_infos.json  output/masstree_cons_rbv_fastcheck.json masstree_rbv_consistency