# LOG_LEVEL=DEBUG python test_ng.py \
#     --tag xyw_test_map_1 \
#     --debug \
#     --test-type map \
#     --temp-dir ./tests/ \
#     --scee-dir /home/xiayanwen/research/SCEE/silent_cpu_error_test/ \
#     --llvm-dir ~/.local/opt/llvm16/ \
#     --t-build 16 \
#     --t-test 16 \
#     --output output_new_1.json > test_map_1.log 2>&1

# LOG_LEVEL=DEBUG python test_ng.py \
#     --debug \
#     --test-type test \
#     --temp-dir ./tests/ \
#     --scee-dir ./fake-test/ \
#     --llvm-dir ~/.local/opt/llvm16/ \
#     --t-build 16 \
#     --t-test 16 \
#     --output output.json > test1.log 2>&1

# LOG_LEVEL=DEBUG python test_ng.py \
#     --tag test_float1 \
#     --debug \
#     --test-type float_test \
#     --temp-dir ./tests/ \
#     --scee-dir ./test_case/computational/float/float1simple/ \
#     --llvm-dir ~/.local/dev/llvm16/ \
#     --t-build 1 \
#     --t-test 1 \
#     --output output.json > float_test1.log 2>&1

# LOG_LEVEL=DEBUG python test_ng.py \
#     --tag test_float2 \
#     --debug \
#     --test-type float_test2 \
#     --temp-dir ./tests/ \
#     --scee-dir ./test_case/computational/float/float2vector/ \
#     --llvm-dir ~/.local/dev/llvm16/ \
#     --t-build 16 \
#     --t-test 16 \
#     --output output.json > float_test2.log 2>&1

# LOG_LEVEL=DEBUG python test_ng.py \
#     --tag simd1_test \
#     --debug \
#     --test-type simd1_test \
#     --temp-dir ./tests/ \
#     --scee-dir ./test_case/computational/simd/simd1simple/ \
#     --llvm-dir ~/.local/dev/llvm16/ \
#     --t-build 16 \
#     --t-test 16 \
#     --output output.json > simd1_test.log 2>&1

# LOG_LEVEL=DEBUG python test_ng.py \
#     --tag simd3_test \
#     --debug \
#     --test-type simd3_test \
#     --temp-dir ./tests/ \
#     --scee-dir ./test_case/computational/simd/simd3memcpy/ \
#     --llvm-dir ~/.local/dev/llvm16/ \
#     --t-build 16 \
#     --t-test 16 \
#     --output output.json > simd3_test.log 2>&1

# LOG_LEVEL=DEBUG python test_ng.py \
#     --tag simd4_test \
#     --debug \
#     --test-type simd4_test \
#     --temp-dir ./tests/ \
#     --scee-dir ./test_case/computational/simd/simd4crc/ \
#     --llvm-dir ~/.local/dev/llvm16/ \
#     --t-build 16 \
#     --t-test 16 \
#     --output output.json > simd4_test.log 2>&1

LOG_LEVEL=INFO python test_ng.py \
    --tag hashmap_cache_all \
    --debug \
    --test-type hashmap \
    --temp-dir ./tests/ \
    --scee-dir /home/xiayanwen/scee/orth/ \
    --llvm-dir ~/.local/dev/llvm16/ \
    --t-build 20 \
    --t-test 20 \
    --category computational \
    --output output_hashmap_all.json > hashmap_all.log 2>&1

# LOG_LEVEL=DEBUG python test_ng.py \
#     --tag cache1 \
#     --debug \
#     --test-type cache1 \
#     --temp-dir ./tests/ \
#     --scee-dir  ./test_case/consistency/cache/cache1 \
#     --llvm-dir ~/.local/dev/llvm16/ \
#     --t-build 1 \
#     --t-test 1 \
#     --output output.json > cache1.log 2>&1

# LOG_LEVEL=DEBUG python test_ng.py \
#     --tag cache2 \
#     --debug \
#     --test-type cache2 \
#     --temp-dir ./tests/ \
#     --scee-dir  ./test_case/consistency/cache/cache2 \
#     --llvm-dir ~/.local/dev/llvm16/ \
#     --t-build 1 \
#     --t-test 1 \
#     --output output.json > cache2.log 2>&1
