#! /bin/bash

LOG_LEVEL=INFO python test_ng.py \
    --tag hashmap_comp_orthrus_test \
    --debug \
    --test-type hashmap \
    --temp-dir ./tests/ \
    --scee-dir /home/xiayanwen/scee/orth/ \
    --llvm-dir ~/.local/dev/llvm16/ \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions both \
    --ft-type orthrus \
    --output output/hashmap_comp_orthrus_test.json > logs/hashmap_comp_orthrus_test.log 2>&1

    
# LOG_LEVEL=INFO python test_ng.py \
#     --tag hashmap_comp_orthrus \
#     --debug \
#     --test-type hashmap \
#     --temp-dir ./tests/ \
#     --scee-dir /home/xiayanwen/scee/orth/ \
#     --llvm-dir ~/.local/dev/llvm16/ \
#     --t-build 40 \
#     --t-test 40 \
#     --category computational \
#     --fj-functions both \
#     --ft-type orthrus \
#     --output output/hashmap_comp_orthrus.json > logs/hashmap_comp_orthrus.log 2>&1

# wait

# echo "second start"

# LOG_LEVEL=INFO python test_ng.py \
#     --tag hashmap_cons_orthrus \
#     --debug \
#     --test-type hashmap \
#     --temp-dir ./tests/ \
#     --scee-dir /home/xiayanwen/scee/orth/ \
#     --llvm-dir ~/.local/dev/llvm16/ \
#     --t-build 40 \
#     --t-test 40 \
#     --category consistency \
#     --fj-functions onlyapp \
#     --ft-type orthrus \
#     --output output/hashmap_cons_orthrus.json > logs/hashmap_cons_orthrus.log 2>&1

# LOG_LEVEL=INFO python test_ng.py \
#     --tag hashmap_all_cons \
#     --debug \
#     --test-type hashmap \
#     --temp-dir ./tests/ \
#     --scee-dir /home/xiayanwen/scee/orth/ \
#     --llvm-dir ~/.local/dev/llvm16/ \
#     --t-build 20 \
#     --t-test 20 \
#     --category consistency \
#     --output output_hashmap_all_consistency.json > hashmap_all_consistency.log 2>&1