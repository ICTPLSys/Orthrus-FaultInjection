#!/usr/bin/env bash

set -xe

ROOT=$(dirname $(realpath $0))

$ROOT/latency_vs_pXX_base.sh \
  $ROOT/../build/tests/lsmtree/latency/lsmtree_socket_latency_pXX_raw \
  raw
