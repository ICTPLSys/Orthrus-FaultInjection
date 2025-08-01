#! /bin/bash

LOG_LEVEL=INFO python test_ng.py \
    --tag wc_comp_orthrus_lite \
    --debug \
    --test-type wordcount \
    --temp-dir ./tests/ \
    --scee-dir /home/xiayanwen/scee/wordcount/ \
    --llvm-dir ~/.local/dev/llvm16/ \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions both \
    --ft-type orthrus \
    --test_mode lite \
    --output output/wc_comp_orthrus_lite.json > logs/wc_comp_orthrus_lite.log 2>&1

# python3 parse.py ./tests/wc_comp_orthrus/cache/func_fj_infos.json ./output/wc_comp_orthrus.json wordcount
# wait

# echo "second start"

# LOG_LEVEL=INFO python test_ng.py \
#     --tag wc_cons_orthrus \
#     --debug \
#     --test-type wordcount \
#     --temp-dir ./tests/ \
#     --scee-dir /home/xiayanwen/scee/wordcount/ \
#     --llvm-dir ~/.local/dev/llvm16/ \
#     --t-build 40 \
#     --t-test 40 \
#     --category consistency \
#     --fj-functions both \
#     --ft-type orthrus \
#     --output output/wc_cons_orthrus.json > logs/wc_cons_orthrus.log 2>&1