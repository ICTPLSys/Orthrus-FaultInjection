import re

import numpy as np

mm = {}

pat = re.compile(r"diff\[(?P<scope>\w+)\]\[(?P<ns>\w+)\]: (?P<ticks>\d+)")
def worker(file):
    global mm

    with open(file, "r", encoding="utf8") as f:
        lines = f.read().splitlines()

    for line in lines:
        match = pat.match(line)
        if not match: continue
        ns = match.group("ns")
        scope = match.group("scope")
        ticks = match.group("ticks")

        if (ns == "validator" or scope == "validator"): continue

        if scope not in mm:
            mm[scope] = {}

        if ns not in mm[scope]:
            mm[scope][ns] = []

        mm[scope][ns].append(int(ticks))


worker("log.raw")
worker("log.scee")

for scope, scope_value in mm.items():
    for ns, ns_value in scope_value.items():
        print(scope, ns, np.average(ns_value))
    print("")

