from utils.ktypes import *
from typing import Dict, List

import re
import logging
import itertools
import traceback

logger = logging.getLogger(__name__)

IS_OUT = False

IGNORED_FJ_FUNCTIONS = [
    re.compile(".*PASS_TWO_SCEE.*writer_t.*copy.*[(].*[)].*"),
    re.compile(".*platform.*actor_run.*"),
    re.compile(r".*NO_.*"),
]


# Must be full match since we use pat.match()
# TODO:(yanwen) make this a parameter from command line
# FJ_FUNCTIONS = [
#     # re.compile(r".*_ZN9val.*"),
#     # re.compile(r".*_ZNK9val.*"),
#     # re.compile(r".*_ZTWN4scee.*"),
#     # re.compile(r".*_ZN4scee.*"),
#     # re.compile(r".*_ZNK4scee.*"),
#     re.compile(r".*_ZN3app.*"),
#     re.compile(r".*_ZNK3app.*"),
# ]

FJ_FUNCTIONS = None

def init_fj_functions(fj_functions: str):
    global FJ_FUNCTIONS
    if fj_functions == "onlyapp":
        logger.info(f"Using only app functions")
        FJ_FUNCTIONS = [
            re.compile(r".*_ZN3app.*"),
            re.compile(r".*_ZNK3app.*"),
        ]
    elif fj_functions == "onlyscee":
        logger.info(f"Using only scee functions")
        FJ_FUNCTIONS = [
            re.compile(r".*_ZTWN4scee.*"),
            re.compile(r".*_ZN4scee.*"),
            re.compile(r".*_ZNK4scee.*"),
        ]
    elif fj_functions == "both":
        logger.info(f"Using both app and scee functions")
        FJ_FUNCTIONS = [
            re.compile(r".*_ZTWN4scee.*"),
            re.compile(r".*_ZN4scee.*"),
            re.compile(r".*_ZNK4scee.*"),
            re.compile(r".*_ZN3app.*"),
            re.compile(r".*_ZNK3app.*"),
        ]
    elif fj_functions == "wctest":
        logger.info(f"Using wordcount test functions")
        FJ_FUNCTIONS = [
            re.compile(r".*app.*calc_standard_deviation.*"),
            re.compile(r".*app.*calc_average_frequency.*"),
            re.compile(r".*app.*calc_average_length.*"),
            re.compile(r".*app.*calc_each_word_frequency.*"),
        ]
    else:
        FJ_FUNCTIONS = None

def parse_inspect_build_result(output: KOutput) -> Dict[str, FJInfoType]:
    # Current Machine Function: _Z3bari  (Real: bar(int))
    ss = output.stdout
    logger.debug(f"start parsing inspect build result")

    pat1 = re.compile(r"^Current Machine Function: (.+)\s+\[Real: (.+)\]$")
    pat2 = re.compile(r"^Current Machine Function: (.+)$")

    # [Inst.34]: $rcx = MOV64rr $rbp
    pat_inst = re.compile(r"\[Inst\.(\d+)\]: (.+)")

    f_info: Dict[str, FJInfoType] = {}

    func_name = None
    for line in ss.splitlines():
        # check new machine function
        isCurrentFuncDeclare = False
        if (matches := pat1.match(line)):
            func_name = matches.group(1).strip()
            isCurrentFuncDeclare = True
        elif (matches := pat2.match(line)):
            func_name = matches.group(1).strip()
            isCurrentFuncDeclare = True

        # if func_name and isCurrentFuncDeclare:
        #     print(line)
        #     print(func_name)
        #     input("continue?")

        if isCurrentFuncDeclare:
            logger.debug(f"Find a new function: {func_name}")
            f_info[func_name] = {
                "insts": [],
                "inst_count": 0,
            }
            continue

        # check [Inst.N] line
        if (matches := pat_inst.match(line)):
            idx = int(matches.group(1))
            inst = matches.group(2)
            _tmp: PcInst = {
                "pc": idx,
                "inst": inst,
            }
            f_info[func_name]["insts"].append(_tmp)
            # assert (cnt := f_info[func_name]["inst_count"]) == idx, f"Mismatch Inst Count {cnt} != {idx}"
            f_info[func_name]["inst_count"] += 1

    ret = {}
    for fname, info in f_info.items():
        SHOULD_IGNORE = False
        for pat in IGNORED_FJ_FUNCTIONS:
            if (pat.match(fname)):
                SHOULD_IGNORE = True
                break
        if SHOULD_IGNORE:
            # logger.info(f"Ignoring function: {fname}")
            continue

        SHOULD_USE = False
        for pat in FJ_FUNCTIONS:
            if (pat.match(fname)):
                SHOULD_USE = True
                break
        if not SHOULD_USE:
            continue
        logger.debug(f"Record function: {fname}")
        ret[fname] = info

    # input("Please check the function list, enter to continue...")
    return ret

def parse_profile_build_result(ss: str) -> List[FJInfoType]:
    # [FaultInject] Inst.0: frame-setup PUSH64r killed $r15, implicit-def $rsp, implicit $rsp
    pat1 = re.compile(r"\[FaultInject\]\s+Inst\.(\d+): (.+)")
    # [FaultInfo]: fpu_calc ADDSDrr
    pat2 = re.compile(r"\[FaultInfo\]: (.+)")
    fj_insts: List[FJInfoType] = []
    for line in ss.splitlines():
        if (matches := pat1.match(line)):
            fj_insts.append({
                "pc": int(matches.group(1)),
                "inst": matches.group(2),
            })

    return fj_insts

def parse_profile_injection_result(
        profile_result: RunResultType,
        injection_result: RunResultType,
        mask_result: RunResultType = { 'error': RunResult.Ignored, 'data': "" },
    ) -> FJTestResult:
    # profile x injection
    result_map = {
        RunResult.Success: {
            RunResult.Success: {
                RunResult.Ignored: SCEECheckResult.NotExecuted,

                RunResult.Success: SCEECheckResult.NotExecuted,
            },
        },
        RunResult.SigTrap: {
            # ok
            RunResult.Success: {
                RunResult.Ignored: SCEECheckResult.Unknown,

                RunResult.Success: SCEECheckResult.Unknown,
            },
            # CP
            RunResult.ErrorDetected:  {
                RunResult.Ignored: SCEECheckResult.SCEE_Detected,

                RunResult.Success: SCEECheckResult.SCEE_Detected_NonFatal,
                RunResult.TestFailed: SCEECheckResult.SCEE_Detected,
            },
            # OP
            RunResult.TestFailed:     {
                RunResult.Ignored: SCEECheckResult.SCEE_Not_Detected,

                RunResult.TestFailed: SCEECheckResult.SCEE_Not_Detected,
            },
            # -1
            RunResult.Timeout:        SCEECheckResult.NoneSCEE,
            RunResult.SegmentFault:   SCEECheckResult.NoneSCEE,
            RunResult.FPError:        SCEECheckResult.NoneSCEE,
            RunResult.Terminate:      SCEECheckResult.NoneSCEE,
            RunResult.Abort:      SCEECheckResult.NoneSCEE,
            RunResult.UnknownRetcode: SCEECheckResult.NoneSCEE,
        },
    }


    try:
        p_err = profile_result['error']
        i_err = injection_result['error']
        m_err = mask_result['error']

        def error_process(result: RunResultType):
            logger.error(f"{profile_result}")
            logger.error(f"{injection_result}")
            logger.error(f"{mask_result}")
            logger.error(f"Unknown profile error: {result}")
            return SCEECheckResult.Unknown

        tmp = result_map.get(p_err, SCEECheckResult.Unknown)
        if type(tmp) is SCEECheckResult:
            return tmp
        if not tmp:
            return error_process(profile_result)

        tmp = tmp.get(i_err, SCEECheckResult.Unknown)
        if type(tmp) is SCEECheckResult:
            return tmp
        if not tmp:
            return error_process(injection_result)

        tmp = tmp.get(m_err, SCEECheckResult.Unknown)
        if type(tmp) is SCEECheckResult:
            return tmp
        if not tmp:
            return error_process(mask_result)
    except Exception as e:
        logger.error(e)
        logger.error(traceback.format_exc())
        return SCEECheckResult.Unknown

    return tmp


def parse_run_result(rc: int, out: str, err: str) -> RunResultType:
    ret = {
        "error": RunResult.Unknown,
        "data": {
            "rc": rc,
            "out": out.splitlines(),
            "err": err.splitlines(),
        }
    }

    if rc == -5:
        # sigtrap, int 3 triggered
        ret['error'] = RunResult.SigTrap
        logger.debug(f"SigTrap, int 3 triggered")
    elif rc == -4:  # kill means timeout
        ret['error'] = RunResult.Timeout
    elif rc == -11:
        ret['error'] = RunResult.SegmentFault
    elif rc == -8:
        ret['error'] = RunResult.FPError
    elif rc == -6:
        ret['error'] = RunResult.Abort
    elif rc == -15:
        ret['error'] = RunResult.Terminate
    elif rc == 0:
        ret['error'] = RunResult.Success
    else:
        ret['error'] = RunResult.UnknownRetcode

    out = out.splitlines()
    err = err.splitlines()

    for line in itertools.chain(out, err):
        # Not detected
        if re.search(r"\bError Not Detected\b", line, re.IGNORECASE):
            ret['error'] = RunResult.TestFailed
            return ret
        if re.search(r"\bValidation Failed\b", line, re.IGNORECASE):
            ret['error'] = RunResult.ErrorDetected
            return ret
        if re.search(r"\bTest Passed\b", line, re.IGNORECASE):
            ret['error'] = RunResult.Success
            return ret
        #  [$PATH/lib/cache.cc:47] Assertion failed:false
        # detected_pat = re.compile(r"\[(.+\..+:\d+)\] Assertion failed")
        # if (matches := detected_pat.match(line)):
        #     ret['error'] = RunResult.ErrorDetected
        #     ret['data'] = {
        #         "log": matches.group(0),
        #         "file": matches.group(1),
        #     }
        #     return ret

    # something wrong
    # logger.debug(rc)
    # logger.debug(out)
    # logger.debug(err)
    # input("Unknown Error, Continue?")

    return ret

def get_fault_type_from_build_result(ss: str) -> FaultInfoByUnit:
    # [FaultInfo]: fpu calc ADDSDrr
    # [FaultInfo]: alu ADD
    # get fpu calc ADDSDRR seperately into a variable
    for line in ss.splitlines():
        match = re.match(r"\[FaultInfo\]:\s*(\S+)\s*(\S+)(?:\s*(\S+))?", line)
        # print(f"match: {match}")
        
        if match:
            fault_type_by_unit = match.group(1)
            fault_type_by_instr = match.group(2)
            instr_name = match.group(3) if match.group(3) else ""

            return FaultInfoByUnit(
                fault_type_by_unit=fault_type_by_unit,
                fault_type_by_instr=fault_type_by_instr,
                instr_name=instr_name
            )

    return FaultInfoByUnit(
        fault_type_by_unit="",
        fault_type_by_instr="",
        instr_name=""
    )



#     # re.compile(r".*_ZTWN4scee.*"),
#     # re.compile(r".*_ZN4scee.*"),
#     # re.compile(r".*_ZNK4scee.*"),
#     re.compile(r".*_ZN3app.*"),
#     re.compile(r".*_ZNK3app.*"),