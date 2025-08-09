#!/bin/bash

# table2_fastcheck.sh
# NOTE: THIS SCRIPT IS ONLY FOR FAST CHECK, NOT FOR FULL TESTING. FOR FULL TESTING, USE table2_full.sh
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

# cd framework path
pushd $FRAMEWORK_PATH

# Run FJ for HASHMAP-Orthrus
./scripts/memcached_orthrus_fastcheck.sh

# # wait
# # # Run FJ for HASHMAP-RBV
# ./scripts/memcached_rbv_fastcheck.sh

# # wait
# # # Run FJ for MASSTREE-Orthrus
./scripts/masstree_orthrus_fastcheck.sh

# # wait
# # # Run FJ for MASSTREE-RBV
# ./scripts/masstree_orthrus_fastcheck.sh

# # wait
# # # Run FJ for LSMTree-Orthrus
./scripts/lsmtree_orthrus_fastcheck.sh

# # wait
# # # Run FJ for LSMTree-RBV
./scripts/lsmtree_rbv_fastcheck.sh

# # wait
# # # Run FJ for WordCount-Orthrus
./scripts/wordcount_orthrus_fastcheck.sh

# wait
# # Run FJ for WordCount-RBV
./scripts/wordcount_rbv_fastcheck.sh

# wait
popd

###### END ######