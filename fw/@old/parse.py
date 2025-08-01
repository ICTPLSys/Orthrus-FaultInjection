import json
import argparse
import logging
from pprint import pprint
from enum import Enum

import subprocess

logger = logging.getLogger(__name__)

def demangle(name):
    args = ['c++filt', name]
    pipe = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    stdout, _ = pipe.communicate()
    demangled = stdout.decode('utf-8').splitlines()
    return demangled[0]


class ErrorTypeCollection:
    err_scee_not_detected = 0
    err_scee_detected = 0
    err_unknown = 0
    err_crash = 0
    ok_not_run = 0

EC = ErrorTypeCollection()

INST_STATE = {}

def collect_result(res_profile, res_injection, pc_fj_info):
    global EC, INST_STATE

    pc = pc_fj_info["pc"]
    func = pc_fj_info["function"]
    inst = pc_fj_info['inst']

    if res_profile == "SigTrap":
        tmp = INST_STATE.get(inst, [])
        tmp.append("triggered")
        print(inst, "triggered")
        INST_STATE[inst] = tmp

        if res_injection == "ErrorDetected":
            # SCEE_DETECTED
            EC.err_scee_detected += 1
            print(pc, func, inst)
            # return True
        elif res_injection == "TestFailed":
            # SCEE_NOT_DETECTED
            EC.err_scee_not_detected += 1
            # return True
        elif res_injection == "Success":
            # Unknown
            EC.err_unknown += 1
            return False
        else:
            EC.err_crash += 1
    elif res_profile == "Success":
        EC.ok_not_run += 1

        tmp = INST_STATE.get(inst, [])
        tmp.append("not run")
        INST_STATE[inst] = tmp
        print(inst, "not run")

    return False

class Pattern(Enum):
    Unknown = "unknown"

    TestAndJcc = "test_and_jmp"
    CmpAndJcc = "cmp_and_jmp"
    DoAndSetBit = "do_and_set_bit"
    XorToSetZero = "just_xor"
    AVXInst = "just_avx"
    JMPInst = "just_jmp"
    JCCInst = "just_jcc"
    LeaInst = "just_lea"
    CallPrint = "call_print_func"
    CallPushPacket = "call_push_packet"
    CallTestInternal = "call_testing_internal"
    PushInst = "just_push"
    KILL = "just_kill"
    CallMutex = "call_mutex"

    # call pthread_mutex_unlock 
    # call

    ShouldIgnore = "ignored"

    
def classify_inst(inst: str, next_inst: str):
    cur = inst.lower()
    nxt = next_inst.lower()
    if "test" in cur and "jcc" in nxt:
        # return Pattern.Unknown
        return Pattern.TestAndJcc
    elif "= lea" in cur:
        return Pattern.LeaInst
    elif "cmp" in cur and "jcc" in nxt:
        return Pattern.CmpAndJcc
    elif "setccr" in nxt:
        return Pattern.DoAndSetBit
    elif "xmm" in cur or "ymm" in cur or "zmm" in cur:
        return Pattern.AVXInst
    elif "jmp" in cur:
        return Pattern.JMPInst
    elif "jcc" in cur:
        return Pattern.JCCInst
    elif "xor" in cur:
        return Pattern.XorToSetZero
    elif "call" in cur:
        if "printf" in cur or "put" in cur:
            return Pattern.CallPrint
        elif "pushpacket" in cur:
            return Pattern.CallPushPacket
        elif "testing" in cur and "internal" in cur:
            return Pattern.CallTestInternal
        elif "mutex" in cur:
            return Pattern.CallMutex
    elif "push" in cur:
        return Pattern.PushInst
    elif "= kill renamable" in cur:
        return Pattern.ShouldIgnore
    elif "inlineasm" in cur:
        return Pattern.ShouldIgnore

    return Pattern.Unknown


def main(args):
    with open(args.file, "r") as f:
        data = json.load(f)

    mm = {}
    dup = {}
    dup_pat = {}

    for fname, pc_fj_info_list in data.items():
        logger.info(f"Parsing {fname}")

        mm[fname] = []

        for pc_fj_info in pc_fj_info_list:
            if pc_fj_info == None: continue
            pc = int(pc_fj_info['pc'])
            inst = pc_fj_info['inst']
            prev_inst = ""
            next_inst = ""

            if (profile_build := pc_fj_info.get("profile_build", None)):
                for idx, line in enumerate(profile_build):
                    if inst in line:
                        next_inst = profile_build[idx + 1]
                        break
                    prev_inst = line

            function_name = pc_fj_info['function']
            assert function_name == fname, f"Mismatch between {function_name} and {fname}"
            result = pc_fj_info['result']

            res_profile = result['profile']["error"]
            res_injection = result['injection']["error"]

            tmp = {
                "pc": pc,
                "profile": res_profile,
                "injection": res_injection,
            }
            mm[fname].append(tmp)

            key = (res_profile, res_injection)
            d = dup.get(key, 0) 
            dup[key] = d + 1

            if collect_result(res_profile, res_injection, pc_fj_info):
                pat = classify_inst(inst, next_inst)
                cnt = dup_pat.get(pat, 0)
                dup_pat[pat] = cnt + 1

                # if True or pat == Pattern.Unknown:
                if pat == Pattern.Unknown:
                    print(demangle(fname), pc)
                    print(prev_inst)
                    print("\t" + inst)
                    print(next_inst)
                    print("")
                else:
                    pass
                    #print(pat)
                    #print(inst, next_inst)
                    #print("")



    # pprint(mm)

    pprint(dup)
    pprint(dup_pat)

    print(EC.__dict__)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("file", help="the file to parse")

    args = parser.parse_args()

    main(args)
