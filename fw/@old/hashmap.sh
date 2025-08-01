#!/bin/bash

set -x
set -e

OUT_DIR="results"

mkdir -p $OUT_DIR

worker() {
    set +e
    find /dev/shm -type f -exec rm {} \;
    set -e

    fault_name=$1
    fault_template=$2

    result_file=$OUT_DIR/results.hashmap.$fault_name.json
    parse_file=$OUT_DIR/parse.hashmap.$fault_name.log

    echo --------------------------------------------
    echo working on $fault_name

    set +e
    killall operation_test; 
    set -e

    time python3 test.py \
        --test-exec operation/operation_test \
        --scee-dir ../scee_asplos_hashmap \
        --threads 16 \
        --template $fault_template

    mv result.json $result_file
    python3 parse.py $result_file > $parse_file
}

worker noop_and_branching  fault_nop.json
worker bitflip_1           fault_bitflip_1.json
worker bitflip_2c          fault_bitflip_2c.json
