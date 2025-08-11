#! /bin/bash

rm -rf tests/masstree_comp_rbv_full/
rm -rf tests/masstree_cons_rbv_full/

LOG_LEVEL=INFO python test_ng.py \
    --tag masstree_comp_rbv_full \
    --debug \
    --test-type masstree \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/masstree_rbv/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions both \
    --ft-type rbv_full \
    --output output/masstree_comp_rbv_full.json > logs/masstree_comp_rbv_full.log 2>&1

wait

LOG_LEVEL=INFO python test_ng.py \
    --tag masstree_cons_rbv_full \
    --debug \
    --test-type masstree \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/masstree_rbv/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category consistency \
    --fj-functions both \
    --ft-type rbv_full \
    --output output/masstree_cons_rbv_full.json > logs/masstree_cons_rbv_full.log 2>&1

python3 parse.py tests/masstree_comp_rbv_full/cache/func_fj_infos.json  output/masstree_comp_rbv_full.json masstree_rbv_computational
python3 parse.py tests/masstree_cons_rbv_full/cache/func_fj_infos.json  output/masstree_cons_rbv_full.json masstree_rbv_consistency
