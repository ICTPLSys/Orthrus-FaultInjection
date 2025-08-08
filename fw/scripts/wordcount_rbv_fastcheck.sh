#! /bin/bash

rm -rf tests/wc_comp_rbv_fastcheck/
rm -rf tests/wc_cons_rbv_fastcheck/

LOG_LEVEL=INFO python test_ng.py \
    --tag wc_comp_rbv_fastcheck \
    --debug \
    --test-type wordcount_rbv \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/wordcount_rbv/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 20 \
    --t-test 20 \
    --category computational \
    --fj-functions onlyapp \
    --ft-type rbv_fastcheck \
    --test_mode lite \
    --output output/wc_comp_rbv_fastcheck.json > logs/wc_comp_rbv_fastcheck.log 2>&1

wait

LOG_LEVEL=INFO python test_ng.py \
    --tag wc_cons_rbv_fastcheck \
    --debug \
    --test-type wordcount_rbv \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/wordcount_rbv/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 20 \
    --t-test 20 \
    --category consistency \
    --fj-functions onlyapp \
    --ft-type rbv_fastcheck \
    --test_mode lite \
    --output output/wc_cons_rbv_fastcheck.json > logs/wc_cons_rbv_fastcheck.log 2>&1

python3 parse.py tests/wc_comp_rbv_fastcheck/cache/func_fj_infos.json  output/wc_comp_rbv_fastcheck.json wordcount_rbv_computational
python3 parse.py tests/wc_cons_rbv_fastcheck/cache/func_fj_infos.json  output/wc_cons_rbv_fastcheck.json wordcount_rbv_consistency