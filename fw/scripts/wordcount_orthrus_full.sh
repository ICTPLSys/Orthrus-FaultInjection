#! /bin/bash

rm -rf tests/wc_comp_orthrus_full/
rm -rf tests/wc_cons_orthrus_full/

LOG_LEVEL=INFO python test_ng.py \
    --tag wc_comp_orthrus \
    --debug \
    --test-type wordcount \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/wordcount_orthrus/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions both \
    --ft-type orthrus_full \
    --output output/wc_comp_orthrus_full.json > logs/wc_comp_orthrus_full.log 2>&1

wait

LOG_LEVEL=INFO python test_ng.py \
    --tag wc_cons_orthrus_full \
    --debug \
    --test-type wordcount \
    --temp-dir ./tests/ \
    --scee-dir ${CWD}/fj_targets/wordcount_orthrus/ \
    --llvm-dir ${LLVM_INSTALL_PATH} \
    --t-build 40 \
    --t-test 40 \
    --category consistency \
    --fj-functions both \
    --ft-type orthrus_full \
    --output output/wc_cons_orthrus_full.json > logs/wc_cons_orthrus_full.log 2>&1

python3 parse.py tests/wc_comp_orthrus_full/cache/func_fj_infos.json  output/wc_comp_orthrus_full.json wordcount_orthrus_computational
python3 parse.py tests/wc_cons_orthrus_full/cache/func_fj_infos.json  output/wc_cons_orthrus_full.json wordcount_orthrus_consistency