import json
import re
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("-i", "--input")
parser.add_argument("-o", "--output")

args = parser.parse_args()

with open(args.input, "r") as f:
    lines = f.read().splitlines()

item = None

pat1 = re.compile(r"execution time: (\d+), throughput: (.+)")
pat2 = re.compile(r"avg: ([^ ]+) us, p90: ([^ ]+) us, p95: ([^ ]+) us, p99: ([^ ]+) us")

idx = 0
data = []
while idx < len(lines):
    line = lines[idx]
    if "Send" in line:
        if item:
            data.append(item)
        item = {}
    elif "execution time" in line:
        matches = pat1.match(line)
        item["throughput"] = float(matches[2])
        item["duration"] = float(matches[1])
    elif "latency_net" in line:
        idx += 1
        line = lines[idx]
        matches = pat2.match(line)
        item["latency_net"] = {
            "avg": float(matches[1]),
            "p90": float(matches[2]),
            "p95": float(matches[3]),
            "p99": float(matches[4]),
        }
    elif "latency_req" in line:
        idx += 1
        line = lines[idx]
        matches = pat2.match(line)
        item["latency_req"] = {
            "avg": float(matches[1]),
            "p90": float(matches[2]),
            "p95": float(matches[3]),
            "p99": float(matches[4]),
        }
    idx += 1

#  print(data)

with open(args.output, "w") as f:
    json.dump(data, f, indent=2, ensure_ascii=False)
