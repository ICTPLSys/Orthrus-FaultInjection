import os
import re
import json
import argparse
from typing import List, Dict, Any
from pathlib import Path
from toolz.curried import map, compose, pipe, take, reduce, filter
from toolz.recipes import partitionby



parser = argparse.ArgumentParser()
parser.add_argument("--input-raw", required=True, help="lsmtree-memory_status-raw.log")
parser.add_argument("--input-scee", required=True, help="lsmtree-memory_status-scee.log")
parser.add_argument("--input-rbv", required=True, help="lsmtree-memory_status-rbv.log")

args = parser.parse_args()

raw_mem = Path(args.input_raw)
scee_mem = Path(args.input_scee)
rbv_mem = Path(args.input_rbv)

# DIVISION_MARK = "---- started ----"

def parser(mem: Path):
    pat = re.compile(r"VmRSS:\s+(\d+) kB")

    def process(line):
        matches = pat.match(line)
        if not matches:
            print("="*10)
            print(line)
            return None
        # print(matches[1])
        ret = int(matches[1])
        return ret

    with open(mem, "r", encoding="utf8") as f:
        data: List[str] = f.read().splitlines()

    # tmp = partitionby(lambda x: x == DIVISION_MARK, data)
    # tmp = list(tmp)
    # init_stage = tmp[0]
    # run_stage = tmp[2]

    run_stage = data

    f = compose(
        map(lambda x: process(x)),
        filter(lambda x: x),
    )

    # init_stage_parsed = list(f(init_stage))
    run_stage_parsed = list(f(run_stage))
    # max_init_stage_mem = max(init_stage_parsed)
    max_run_stage_mem = max(run_stage_parsed)
    avg_run_stage_mem = sum(run_stage_parsed) // len(run_stage_parsed)

    # print("max mem init: ", max_init_stage_mem)
    print("max mem run : ", max_run_stage_mem)
    # print("mem max diff usage:    ", max_run_stage_mem - max_init_stage_mem)
    # print("mem avg diff usage:    ", avg_run_stage_mem - max_init_stage_mem)
    return (
        # max_init_stage_mem,
        max_run_stage_mem,
        avg_run_stage_mem,
        # max_run_stage_mem - max_init_stage_mem,
        # avg_run_stage_mem - max_init_stage_mem,
    )


print("Processing raw")
(
    # raw_max_init_mem,
    raw_max_run_mem,
    raw_avg_run_mem,
    # raw_max_diff_mem,
    # raw_avg_diff_mem,
) = parser(raw_mem)

print("Processing scee")
(
    # scee_max_init_mem,
    scee_max_run_mem,
    scee_avg_run_mem,
    # scee_max_diff_mem,
    # scee_avg_diff_mem,
) = parser(scee_mem)

print("Processing rbv")
(
    # rbv_max_init_mem,
    rbv_max_run_mem,
    rbv_avg_run_mem,
    # rbv_max_diff_mem,
    # rbv_avg_diff_mem,
) = parser(rbv_mem)


diff = lambda a, b: a / b

print("-"*10, " results(peak) ", "-"*10)
print("ratio (scee vs raw): ", diff(scee_max_run_mem, raw_max_run_mem))
print("ratio (rbv vs raw):  ", diff(rbv_max_run_mem, raw_max_run_mem))

print("-"*10, " results(avg) ", "-"*10)
print("ratio (scee vs raw): ", diff(scee_avg_run_mem, raw_avg_run_mem))
print("ratio (rbv vs raw):  ", diff(rbv_avg_run_mem, raw_avg_run_mem))

# print("-"*10, " results(peak run) ", "-"*10)
# print("ratio (rbv vs raw):  ", diff(rbv_max_run_mem, raw_max_run_mem))
# print("ratio (scee vs raw): ", diff(scee_max_run_mem, raw_max_run_mem))
# print("ratio (scee vs rbv): ", diff(rbv_max_run_mem, scee_max_run_mem))
