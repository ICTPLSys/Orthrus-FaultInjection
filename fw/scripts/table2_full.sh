#!/bin/bash

# table2_full.sh
# NOTE: THIS SCRIPT IS ONLY FOR FULL TESTING, NOT FOR FAST CHECK. FOR FAST CHECK, USE table2_fastcheck.sh. FULL TESTING IS VERY SLOW, IT MAY TAKE AROUND 50 HOURS TO COMPLETE. TO HANDLE POSSIBLE FAILURES, YOU CAN RUN THE COMMANDS IN THIS SCRIPT SEQUENTIALLY MANUALLY.
# input:
#   - FRAMEWORK_PATH
#   - LLVM_PATH
#   - HOME_PATH
# output:
#   - stdout: 8 lines, each line corresponds to one line in the table 2
#   - {FRAMEWORK_PATH}/logs/ logs for each line
#   - {FRAMEWORK_PATH}/output/ result file for each line
#   - {FRAMEWORK_PATH}/tests/ testcases, intermediate files

source table2_env.sh

###### START ######

echo "TABLE2 FULL"
echo "Estimated runtime: 50 hours"
echo "Warning: If the scripts failed, we recommend to run the remaining commands in this script sequentially manually"

# cd framework path
pushd $FRAMEWORK_PATH

# Run FJ for HASHMAP-Orthrus
echo "--------------------------------"
echo "Application: Memcached"
# Run FJ for HASHMAP-Orthrus
echo "Orthrus"
./scripts/memcached_orthrus_full.sh

wait
# Run FJ for HASHMAP-RBV
echo "RBV"
./scripts/memcached_rbv_full.sh

wait

echo "--------------------------------"
# # Run FJ for MASSTREE-Orthrus
echo "Application: Masstree"
echo "Orthrus"
./scripts/masstree_orthrus_full.sh

wait

# # Run FJ for MASSTREE-RBV
echo "RBV"
./scripts/masstree_rbv_full.sh

wait

echo "--------------------------------"
# # Run FJ for LSMTree-Orthrus
echo "Application: LSMTree"
echo "Orthrus"
./scripts/lsmtree_orthrus_full.sh

wait

# Run FJ for LSMTree-RBV
echo "RBV"
./scripts/lsmtree_rbv_full.sh

wait

echo "--------------------------------"
# # Run FJ for WordCount-Orthrus
echo "Application: WordCount"
echo "Orthrus"
./scripts/wordcount_orthrus_full.sh

wait

# # Run FJ for WordCount-RBV
echo "RBV"
./scripts/wordcount_rbv_full.sh

wait


popd

###### END ######