import json
import random
from utils.ktypes import *

# FI config file
template = {
    "workmode": "injection",
    "profiles": [],
    "insts_only": [ ],
    "insts_ignore": [ ]
}

profile = {
    "name": "float_demo",
    "match_mode": [],
    "function": "_ZN4Exec3prtEi",
    "select_mode": "all",
    "faults": [ ]
}

fault = {
    "inject_type": "fpu", # unused
    "category": "computational", # make this a parameter from command line
    "repeat_count": 1,
    "types": FJType.nop,  # none, nop, bitflip, stuck_at_0, stuck_at_1
    "pc": 1,
    "options": [],
    "insts_only": [],
    "insts_ignore": []
}


TEST_DEFS: Dict[str, TestDef] = {
    "test": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.test.sh",
            "binary": "build/main",
        },
        "run": {
            "cmd": ["./main"],
        },
    }),
    "hashmap": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.hashmap.sh",
            "binary": "build/benchmarks/redis/redis_faultinject",
        },
        "run": {
            "cmd": ["./redis_faultinject"],
        },
    }),
    "hashmap_rbv": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.hashmap_rbv.sh",
            "binary": "./build/tests/hashmap/faultinjection/hashmap_fj_rbv",
        },
        "run": {
            "cmd": ["./hashmap_fj_rbv"],
        },
    }),
    "map": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.test.sh",
            "binary": "build/tests/hash/fault_injection/fault_injection",
        },
        "run": {
            "cmd": ["./fault_injection", "/home/xiayanwen/research/SCEE/silent_cpu_error_test/shenango/server.config"],
        },
    }),
    "float_test": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.test.sh",
            "binary": "build/float1",
        },
        "run": {
            "cmd": ["./float1"],
        },
    }),
    "float_test2": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.test.sh",
            "binary": "build/float2",
        },
        "run": {
            "cmd": ["./float2"],
        },
    }),
    "simd1_test": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.test.sh",
            "binary": "build/simd1",
        },
        "run": {
            "cmd": ["./simd1"],
        },
    }),
    "simd3_test": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.test.sh",
            "binary": "build/simd3",
        },
        "run": {
            "cmd": ["./simd3"],
        },
    }),
    "simd4_test": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.test.sh",
            "binary": "build/simd4",
        },
        "run": {
            "cmd": ["./simd4"],
        },
    }),
    "cache1": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.test.sh",
            "binary": "build/cc1",
        },
        "run": {
            "cmd": ["./cc1"],
        },
    }),
    "cache2": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.test.sh",
            "binary": "build/cc2",
        },
        "run": {
            "cmd": ["./cc2"],
        },
    }),
    "wordcount": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.wc.sh",
            "binary": "build/benchmarks/word_count/wc",
        },
        "run": {
            "cmd": ["./wc", "-i", "/home/xiayanwen/scee/wordcount/benchmarks/word_count/example.txt"],
        },
    }),
    "wordcount_rbv": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.wc_rbv.sh",
            "binary": "./build/ae/phoenix/faultinjection/phoenix_rbv_fj",
        },
        "run": {
            "cmd": ["./phoenix_rbv_fj", "-i", "/home/xiayanwen/scee/wordcount/benchmarks/word_count/example.txt"],
        },
    }),
    "lsmtree": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.lsmtree.sh",
            "binary": "./build/tests/lsmtree/faultinjection/lsmtree_fj_scee",
        },
        "run": {
            "cmd": ["./lsmtree_fj_scee"],
        },
    }),
    "lsmtree_rbv": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.lsmtree_rbv.sh",
            "binary": "./build/tests/lsmtree/faultinjection/lsmtree_fj_rbv",
        },
        "run": {
            "cmd": ["./lsmtree_fj_rbv"],
        },
    }),
    "masstree": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.masstree.sh",
            "binary": "build/benchmarks/masstree/masstree_faultinject",
        },
        "run": {
            "cmd": ["./masstree_faultinject"],
        },
    }),
    "masstree_rbv": TestDef(**{
        "fj_types": ALL_FJ_TYPES,
        "build": {
            "script": "build.masstree_rbv.sh",
            "binary": "build/benchmarks/masstree/masstree_faultinject",
        },
        "run": {
            "cmd": ["./masstree_fj_rbv"],
        },
    }),
}

def write_fj_conf(filepath: Path, config: dict):
    with open(filepath, "w") as f:
        json.dump(config, f, ensure_ascii=False, indent=2)
    return

def build_fj_conf(
    workMode: WorkMode,
    fj_type: FJType=None,
    TargetFunc: str|None = None,
    pc: int|None = None,
    category: str = "computational"
):
    tmp = deepcopy(template)
    tmp["workmode"] = workMode.value

    tmp_profile = deepcopy(profile)
    if workMode == WorkMode.inspect:
        tmp_profile["function"] = "*"
    else:
        tmp_fault = deepcopy(fault)
        tmp_fault["types"] = fj_type.value
        tmp_fault["category"] = category
        if fj_type == FJType.bitflip1:
            # TODO(kuriko): set the options
            # options = [ bits,  is_continous, ]
            fj_type = "bitflip"
            tmp_fault["options"] = [ 1, 1, ]
            tmp_fault["types"] = "bitflip"
        elif fj_type == FJType.bitflip2:
            fj_type = "bitflip"
            tmp_fault["options"] = [ 2, 1, ]
            tmp_fault["types"] = "bitflip"
        elif fj_type == FJType.bitflip3:
            fj_type = "bitflip"
            tmp_fault["options"] = [ 3, 1, ]
            tmp_fault["types"] = "bitflip"
        elif fj_type == FJType.bitflip_random:
            random_int = random.randint(1, 10)
            fj_type = "bitflip"
            tmp_fault["options"] = [ random_int, 1, ]
            tmp_fault["types"] = "bitflip"
        assert pc != None
        tmp_fault["pc"] = [pc, ]

        assert TargetFunc != None  # FJ on specific function
        tmp_profile["function"] = TargetFunc

        tmp_profile["faults"].append(tmp_fault)

    tmp["profiles"].append(tmp_profile)

    return tmp
