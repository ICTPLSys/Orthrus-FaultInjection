test-lsmtree:
  just test-lsmtree-memory
  just test-lsmtree-throughput
  just test-lsmtree-latency-all
  just test-lsmtree-latency-cdf

test-lsmtree-throughput throughput="50000000": build  # 8 client + 2 server&replica
  #!/bin/bash
  set -e
  echo -e '\n=== test-lsmtree-throughput ==='
  trap 'echo -e "\n\nKilling all background jobs..."; pkill -9 -P $$ &>/dev/null; exit 130' SIGINT SIGTERM
  function worker() {
    NAME=$1
    echo $NAME running
    ./build/ae/lsmtree/throughput/lsmtree_socket_throughput_$NAME \
      >/dev/null 2>&1 &
    sleep 1
    ./build/ae/lsmtree/throughput/lsmtree_socket_throughput_client --total-ops={{throughput}} \
      2>&1 | grep "execution time" &
    wait
    sleep 1
  }

  {
    worker raw
    worker scee
    worker rbv
  } 2>&1 | tee lsmtree-throughput-report.txt
  cp lsmtree-throughput-report.txt results/

test-lsmtree-memory throughput="50000000":
  #!/usr/bin/env bash
  set -e
  echo -e '\n=== test-lsmtree-memory ==='
  trap 'echo -e "\n\nKilling all background jobs..."; pkill -9 -P $$ &>/dev/null; exit 130' SIGINT SIGTERM
  function worker() {
    NAME=$1; echo =============== $NAME =============
    ./build/ae/lsmtree/throughput/lsmtree_socket_profile_mem_$NAME &
    sleep 1
    ./build/ae/lsmtree/throughput/lsmtree_socket_throughput_client --total-ops={{throughput}} &
    wait; sleep 1
  }
  worker raw
  worker scee
  # worker rbv
  just analyze-lsmtree-memory

analyze-lsmtree-memory:
  #!/usr/bin/env bash
  set -e
  {
    echo -e "\n=== Memory Stats ===";
    python3 scripts/memory.py  \
      --input-raw lsmtree-memory_status-raw.log \
      --input-rbv lsmtree-memory_status-rbv.log \
      --input-scee lsmtree-memory_status-scee.log;
  } 2>&1 | tee lsmtree-mem-report.txt
  mkdir -p results
  cp lsmtree-mem-report.txt results/

test-lsmtree-latency-cdf throughput="50000000": build
  #!/usr/bin/env bash
  set -e
  echo -e '\n=== test-lsmtree-latency-cdf ==='
  trap 'echo -e "\n\nKilling all background jobs..."; pkill -9 -P $$ &>/dev/null; exit 130' SIGINT SIGTERM
  function worker() {
    NAME=$1; echo =============== $NAME =============
    ./build/ae/lsmtree/throughput/lsmtree_socket_val_latency_cdf_$NAME &
    sleep 1
    ./build/ae/lsmtree/throughput/lsmtree_socket_throughput_client --total-ops={{throughput}} &
    wait && sleep 1
  }

  worker scee
  worker rbv

  python scripts/validation-latency.py
  echo -e "Output in results/validation-cdf.{pdf|png}"
  mkdir -p results
  cp *.png *.pdf results/


test-lsmtree-latency-all: build
  #!/usr/bin/env bash
  set -e
  echo -e '\n=== test-lsmtree-latency-all ==='
  trap 'echo -e "\n\nKilling all background jobs..."; pkill -9 -P $$ &>/dev/null; exit 130' SIGINT SIGTERM
  function worker() {
    NAME=$1
    LOG=latency_vs_pXX_${NAME}.log
    echo > $LOG
    local THROUGHPUT="100 1000 5000 10000 20000 30000 50000 70000 100000 120000 140000 160000 200000 220000 240000 260000 280000 300000 310000 320000 330000";
    for i in $THROUGHPUT; do
      echo -e "\n====>  $NAME @ $i  <===="
      echo $i >> log
      ./build/ae/lsmtree/latency/lsmtree_socket_latency_pXX_$NAME \
        >/dev/null 2>&1 &
      sleep 1
      ./build/ae/lsmtree/latency/lsmtree_socket_latency_pXX_client \
        --throughput $i 2>&1 | tee /tmp/$LOG &
      wait; sleep 2
      cat /tmp/$LOG >> $LOG
      echo -e "\n\n">> $LOG
    done
  }
  worker raw
  worker scee
  worker rbv
  just analyze-lsmtree-latency
  mkdir -p results
  cp *.png *.pdf results/

analyze-lsmtree-latency:
  python scripts/latency_vs_pXX_parser.py -i latency_vs_pXX_raw.log -o latency_vs_pXX_raw.json
  python scripts/latency_vs_pXX_parser.py -i latency_vs_pXX_scee.log -o latency_vs_pXX_scee.json
  python scripts/latency_vs_pXX_parser.py -i latency_vs_pXX_rbv.log -o latency_vs_pXX_rbv.json
  python scripts/tail-latency.py
  echo -e '\nOutput in tail-latency.{pdf|png}'
