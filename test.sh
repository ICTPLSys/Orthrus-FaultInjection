#!/usr/bin/env bash


set -e

echo -e "Please make sure you have setup the env via [sudo ./init.sh]"

./fw/scripts/table2_fastcheck.sh

./fw/scripts/table2_full.sh