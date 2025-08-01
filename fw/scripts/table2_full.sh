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

HOME_PATH=
FRAMEWORK_PATH=
LLVM_PATH=


###### START ######

# cd framework path
pushd $FRAMEWORK_PATH

# Run FJ for HASHMAP-Orthrus
./scripts/fj_hashmap_orthrus.sh

# Run FJ for HASHMAP-RBV
./scripts/fj_hashmap_rbv.sh

# Run FJ for MASSTREE-Orthrus
./scripts/fj_masstree_orthrus.sh

# Run FJ for MASSTREE-RBV
./scripts/fj_masstree_rbv.sh

# Run FJ for LSMTree-Orthrus
./scripts/fj_lsmtree_orthrus.sh

# Run FJ for LSMTree-RBV
./scripts/fj_lsmtree_rbv.sh

# Run FJ for WordCount-Orthrus
./scripts/fj_wordcount_orthrus.sh

# Run FJ for WordCount-RBV
./scripts/fj_wordcount_rbv.sh

popd

###### END ######