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

HOME_PATH=
FRAMEWORK_PATH=
LLVM_PATH=


###### START ######

# cd framework path
pushd $FRAMEWORK_PATH

# Run FJ for HASHMAP-Orthrus
./scripts/fj_hashmap_orthrus_fastcheck.sh

wait
# Run FJ for HASHMAP-RBV
./scripts/fj_hashmap_rbv_fastcheck.sh

wait
# Run FJ for MASSTREE-Orthrus
./scripts/fj_masstree_orthrus_fastcheck.sh

wait
# Run FJ for MASSTREE-RBV
./scripts/fj_masstree_rbv_fastcheck.sh

wait
# Run FJ for LSMTree-Orthrus
./scripts/fj_lsmtree_orthrus_fastcheck.sh

wait
# Run FJ for LSMTree-RBV
./scripts/fj_lsmtree_rbv_fastcheck.sh

wait
# Run FJ for WordCount-Orthrus
./scripts/fj_wordcount_orthrus_fastcheck.sh

wait
# Run FJ for WordCount-RBV
./scripts/fj_wordcount_rbv_fastcheck.sh

wait
popd

###### END ######