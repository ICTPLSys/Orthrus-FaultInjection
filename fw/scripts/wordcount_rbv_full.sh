#! /bin/bash

rm -rf tests/wc_comp_rbv_full/
rm -rf tests/wc_cons_rbv_full/

source table2_env_runtime.sh

run_with_timeout 28800 LOG_LEVEL=INFO python test_ng.py \
    --tag wc_comp_rbv_full \
    --debug \
    --test-type wordcount_rbv \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/wordcount_rbv/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 30 \
    --t-test 30 \
    --category computational \
    --fj-functions onlyapp \
    --ft-type rbv_full \
    --output output/wc_comp_rbv_full.json > logs/wc_comp_rbv_full.log 2>&1

wait

python3 parse.py tests/wc_comp_rbv_full/cache/func_fj_infos.json  output/wc_comp_rbv_full.json wordcount_rbv_computational