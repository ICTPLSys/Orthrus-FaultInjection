#!/usr/bin/env bash

set -xe

if [[ $1 == "" ]]; then
  echo Usage ./script target logfile
fi

TARGET=$1
LOG=latency_vs_pXX_$2.log

just build

echo > $LOG

for i in  `seq 10 7000 300000`; do
  echo $i >> log
  $TARGET \
      --throughput $i 2>&1 \
      | tee /tmp/$LOG

  cat /tmp/$LOG >> $LOG

  echo >> $LOG
  echo >> $LOG
done
