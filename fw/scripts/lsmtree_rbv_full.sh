#! /bin/bash


rm -rf tests/lsmtree_comp_rbv_full/
rm -rf tests/lsmtree_cons_rbv_full/

LOG_LEVEL=INFO python test_ng.py \
    --tag lsmtree_comp_rbv_full \
    --debug \
    --test-type lsmtree_rbv \
    --temp-dir ./tests/ \
    --scee-dir /home/xiayanwen/scee/newlsm/OrthrusLib-SOSP25 \
    --llvm-dir ~/.local/dev/llvm16/ \
    --t-build 40 \
    --t-test 40 \
    --category computational \
    --fj-functions onlyapp \
    --ft-type rbv_full \
    --output output/lsmtree_comp_rbv_full.json > logs/lsmtree_comp_rbv_full.log 2>&1

# LOG_LEVEL=INFO python test_ng.py \
#     --tag lsmtree_cons_rbv \
#     --debug \
#     --test-type lsmtree_rbv \
#     --temp-dir ./tests/ \
#     --scee-dir /home/xiayanwen/scee/newlsm/OrthrusLib-SOSP25 \
#     --llvm-dir ~/.local/dev/llvm16/ \
#     --t-build 40 \
#     --t-test 40 \
#     --category consistency \
#     --fj-functions onlyapp \
#     --ft-type rbv \
#     --output output/lsmtree_cons_rbv.json > logs/lsmtree_cons_rbv.log 2>&1
