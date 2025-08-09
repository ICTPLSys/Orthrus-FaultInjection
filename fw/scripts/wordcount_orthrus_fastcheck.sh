#! /bin/bash

rm -rf tests/wc_comp_orthrus_fastcheck/
rm -rf tests/wc_cons_orthrus_fastcheck/

LOG_LEVEL=INFO python test_ng.py \
    --tag wc_comp_orthrus_fastcheck \
    --debug \
    --test-type wordcount \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/wordcount_orthrus/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 20 \
    --t-test 20 \
    --category computational \
    --fj-functions both \
    --ft-type orthrus_fastcheck \
    --test_mode lite \
    --output output/wc_comp_orthrus_fastcheck.json > logs/wc_comp_orthrus_fastcheck.log 2>&1

wait

LOG_LEVEL=INFO python test_ng.py \
    --tag wc_cons_orthrus_fastcheck \
    --debug \
    --test-type wordcount \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/wordcount_orthrus/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 20 \
    --t-test 20 \
    --category consistency \
    --fj-functions both \
    --ft-type orthrus_fastcheck \
    --test_mode lite \
    --output output/wc_cons_orthrus_fastcheck.json > logs/wc_cons_orthrus_fastcheck.log 2>&1

python3 parse.py tests/wc_comp_orthrus_fastcheck/cache/func_fj_infos.json  output/wc_comp_orthrus_fastcheck.json wordcount_orthrus_computational
python3 parse.py tests/wc_cons_orthrus_fastcheck/cache/func_fj_infos.json  output/wc_cons_orthrus_fastcheck.json wordcount_orthrus_consistency