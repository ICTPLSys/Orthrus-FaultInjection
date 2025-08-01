import json
from pprint import pprint
import enum

INST_TYPE = {
    "branch": [
        "TAILJMPr64",
        "TAILJMPm64",
        "TAILJMPd64",
        "CALL64pcrel32",
        "CALL64r",
        "CALL64m",
        "JCC_1",
        "JMP_1",
    ],
    "alu": [
        "XOR32rr",
        "RET64",
        "SUB64ri8",
        "CMP64rr",
        "ADD64rr",
        "ADD32rr",
        "XOR64rm",
        "ADD32ri",
        "CMP64mr",
        "TEST32rr",
        "ADD64rm",
        "CMP64mi8",
        "CMP64rm",
        "TEST64rr",
        "CMP32rr",
        "AND32ri",
        "LEA64r",
        "ADD32ri8",
        "OR64rr",
        "SHL64ri",
        "SHL32ri",
        "CMP32rm",
        "CMP32ri8",
        "XOR32rm",
        "INC32r",
        "DIV32r",
        "LEA64_32r",
        "ADD64ri8",
        "CMP32mi8",
        "SETCCr",
        "SUB64ri32",
        "ADD64ri32",
        "SHR64ri",
        "SHR32ri",
        "SHR64ri32",
        "SHR32ri32",
        "SHL64ri32",
        "SHL32ri32",
        "XCHG8rm",
        "XCHG16rm",
        "XCHG32rm",
        "XCHG64rm",
        "TEST8rr",
        "TEST16rr",
        "TEST32rr",
        "TEST64rr",
        "TEST8rm",
        "TEST16rm",
        "TEST32rm",
    ],
    "other": [
        "NOP",
        "RDTSC",
        "RDTSCP",
        "CPUID",
        "POP64r",
        "PUSH64r",
    ],
    "simd": []
}

class ErrorType(enum.Enum):
    SDC_DETECTED = "SDC_DETECTED"
    SDC_NOT_DETECTED = "SDC_NOT_DETECTED"
    MASKED = "MASKED"
    FAIL_STOP = "FAIL_STOP"

def check_profile(result: dict):
    count = {}
    for r in result.values():
        p = r["profile"]
        if p in count:
            count[p] += 1
        else:
            count[p] = 1
    return count

def get_inst_type(n: str, r: dict, info: dict):
    func, pc = n.split("|")
    func = func[3:-1]
    pc = int(pc[3:-1])
    if func not in info:
        print("WARNING:", func)
        return None
    for i in info[func]["insts"]:
        if i["pc"] == pc:
            inst = i["inst"]
            if "=" in inst:
                _, inst = inst.split(" = ")
            tokens = inst.split()
            while tokens[0] in ("frame-setup", "frame-destroy", "nsw", "nuw"):
                tokens = tokens[1:]
            opcode = tokens[0]
            for t, l in INST_TYPE.items():
                if opcode in l:
                    return t
            raise ValueError(f"Unknown opcode: {opcode}")
    print(func, pc)
    assert False



def get_error_type(injection_result: dict):
    err_type = injection_result["error"]
    err_log = injection_result["data"]["err"]
    out_log = injection_result["data"]["out"]

    if any('SDC Not' in line for line in err_log):
        # if any('Validation failed' in line for line in err_log):
        #     return ErrorType.SDC_DETECTED
        return ErrorType.SDC_NOT_DETECTED
    elif any('Validation failed' in line for line in err_log):
        assert err_type == "RunResult.ErrorDetected"
        return ErrorType.SDC_DETECTED
    elif err_type != 'RunResult.Success':
        return ErrorType.FAIL_STOP
    else:
        # if "fault injection test passed" in err_log:
        return ErrorType.MASKED
        # else:
        #     return ErrorType.SDC_NOT_DETECTED
        
class Stats:
    def __init__(self):
        self.dict = dict()
    
    def add(self, hw_type: str, unit_type: str, err_type: ErrorType):
        assert hw_type.startswith("FJType.")
        hw_type = hw_type[7:]
        if hw_type in ('bitflip1', 'bitflip2', 'bitflip3'):
            hw_type = 'bitflip'
        err_type = err_type.value
        self.dict.setdefault((hw_type, unit_type, err_type), 0)
        self.dict[(hw_type, unit_type, err_type)] += 1

    def print(self):
        print(f"{'hw_type':23} {'unit_type':23} {'err_type':23} {'count':23}")
        print("-" * 92)
        for (hw_type, unit_type, err_type), count in sorted(self.dict.items()):
            print(f"{hw_type:23} {unit_type:23} {err_type:23} {count}")

    def stat_sdc_count(self, app_name: str):
        sdc_count = dict()
        sdc_validated_count = dict()
        for (hw_type, unit_type, err_type), count in self.dict.items():
            if err_type == ErrorType.SDC_DETECTED.value:
                sdc_count.setdefault(unit_type, 0)
                sdc_count[unit_type] += count
                sdc_validated_count.setdefault(unit_type, 0)
                sdc_validated_count[unit_type] += count
            elif err_type == ErrorType.SDC_NOT_DETECTED.value:
                sdc_count.setdefault(unit_type, 0)
                sdc_count[unit_type] += count
        # print(sdc_count)
        sdc_validated_prop = dict()
        for unit_type, count in sdc_count.items():
            sdc_validated_prop[unit_type] = sdc_validated_count[unit_type] / count
        # round to three decimal places
        sdc_validated_prop = {k: round(v, 4) for k, v in sdc_validated_prop.items()}
        print("-" * 100)
        print("application_name:", app_name)
        print("validated sdc count:", sdc_validated_count)
        print("total sdc_count by unit_type:", sdc_count)
        print("validation proportion by unit_type:", sdc_validated_prop)
        print("-" * 100)
        return sdc_count

def parse(info_path: str, result_path: str, app_name: str):
    with open(info_path, encoding="ascii") as file:
        info: dict = json.load(file)
    with open(result_path, encoding="ascii") as file:
        result: dict = json.load(file)

    # print(f"{len(info)} functions")
    inst_count = sum(v["inst_count"] for v in info.values())
    # assert inst_count == len(result)
    # print(f"{inst_count} instructions")
    profile_summary = check_profile(result)
    # print(f"profile: {profile_summary}")
    # print(f"{profile_summary['RunResult.SigTrap']} instructions visited")

    stats = Stats()
    for _result_key, result_value in result.items():
        if result_value["profile"] != "RunResult.SigTrap":
            # SigTrap: 该指令可能被执行
            # 跳过不可能被执行的指令
            continue
        for injection in result_value["injection"]:
            name_tokens = injection["name"].split("|")
            if (len(name_tokens) == 6):
                _, _fn_name, _pc, hw_type, unit_type, _inst_name = name_tokens
            elif (len(name_tokens) == 7):
                _, _fn_name, _pc, hw_type, _, unit_type, _inst_name = name_tokens
            else:
                print(name_tokens)
                assert False
            
            err_type = get_error_type(injection["result"])
            stats.add(hw_type, unit_type, err_type)
    stats.print()

    stats.stat_sdc_count(app_name)





if __name__ == "__main__":
    import sys
    parse(sys.argv[1], sys.argv[2], sys.argv[3])
