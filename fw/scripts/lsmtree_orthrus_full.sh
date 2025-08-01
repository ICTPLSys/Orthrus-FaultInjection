#! /bin/bash

LOG_LEVEL=INFO python test_ng.py \
    --tag lsmtree_comp_orthrus \
    --debug \
    --test-type lsmtree \
    --temp-dir ./tests/ \
    --scee-dir /home/xiayanwen/scee/newlsm/OrthrusLib-SOSP25 \
    --llvm-dir ~/.local/dev/llvm16/ \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions both \
    --ft-type orthrus \
    --output output/lsmtree_comp_orthrus.json > logs/lsmtree_comp_orthrus.log 2>&1

# wait

# echo "second start"

# LOG_LEVEL=INFO python test_ng.py \
#     --tag lsmtree_cons_orthrus \
#     --debug \
#     --test-type lsmtree \
#     --temp-dir ./tests/ \
#     --scee-dir /home/xiayanwen/scee/newlsm/OrthrusLib-SOSP25 \
#     --llvm-dir ~/.local/dev/llvm16/ \
#     --t-build 40 \
#     --t-test 40 \
#     --category consistency \
#     --fj-functions both \
#     --ft-type orthrus \
#     --output output/lsmtree_cons_orthrus.json > logs/lsmtree_cons_orthrus.log 2>&1
