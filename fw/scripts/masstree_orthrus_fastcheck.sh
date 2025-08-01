#! /bin/bash

rm -rf tests/masstree_comp_orthrus_fastcheck/
rm -rf tests/masstree_cons_orthrus_fastcheck/

LOG_LEVEL=INFO python test_ng.py \
    --tag masstree_comp_orthrus_fastcheck \
    --debug \
    --test-type masstree \
    --temp-dir ./tests/ \
    --scee-dir /home/xiayanwen/scee/masstree/ \
    --llvm-dir ~/.local/dev/llvm16/ \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions both \
    --ft-type orthrus_fastcheck \
    --test_mode lite \
    --output output/masstree_comp_orthrus_fastcheck.json > logs/masstree_comp_orthrus_fastcheck.log 2>&1

wait 

echo "second start"

LOG_LEVEL=INFO python test_ng.py \
    --tag masstree_cons_orthrus_fastcheck \
    --debug \
    --test-type masstree \
    --temp-dir ./tests/ \
    --scee-dir /home/xiayanwen/scee/masstree/ \
    --llvm-dir ~/.local/dev/llvm16/ \
    --t-build 40 \
    --t-test 40 \
    --category consistency \
    --fj-functions both \
    --ft-type orthrus_fastcheck \
    --test_mode lite \
    --output output/masstree_cons_orthrus_fastcheck.json > logs/masstree_cons_orthrus_fastcheck.log 2>&1

python3 parse.py tests/masstree_comp_orthrus_fastcheck/cache/func_fj_infos.json  output/masstree_comp_orthrus_fastcheck.json masstree_orthrus_computational

python3 parse.py tests/masstree_cons_orthrus_fastcheck/cache/func_fj_infos.json  output/masstree_cons_orthrus_fastcheck.json masstree_orthrus_consistency
