#! /bin/bash

LOG_LEVEL=INFO python test_ng.py \
    --tag hashmap_comp_orthrus_fastcheck \
    --debug \
    --test-type hashmap \
    --temp-dir ./tests/ \
    --scee-dir /home/xiayanwen/scee/orth/ \
    --llvm-dir ~/.local/dev/llvm16/ \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions both \
    --ft-type orthrus_fastcheck \
    --test_mode lite \
    --output output/hashmap_comp_orthrus_fastcheck.json > logs/hashmap_comp_orthrus_fastcheck.log 2>&1

# wait

# echo "second start"

# LOG_LEVEL=INFO python test_ng.py \
#     --tag hashmap_cons_orthrus_fastcheck \
#     --debug \
#     --test-type hashmap \
#     --temp-dir ./tests/ \
#     --scee-dir /home/xiayanwen/scee/orth/ \
#     --llvm-dir ~/.local/dev/llvm16/ \
#     --t-build 40 \
#     --t-test 40 \
#     --category consistency \
#     --fj-functions both \
#     --ft-type orthrus_fastcheck \
#     --test_mode lite \
#     --output output/hashmap_cons_orthrus_fastcheck.json > logs/hashmap_cons_orthrus_fastcheck.log 2>&1
