from enum import Enum
from pathlib import Path
from subprocess import PIPE, Popen, TimeoutExpired
from typing import Dict, List, Any, Optional, Tuple, TypedDict
from dataclasses import dataclass
from pydantic import BaseModel
from copy import deepcopy
from utils.ktypes import *

import logging

logger = logging.getLogger(__name__)

class PcInst(TypedDict):
    pc: int
    inst: str

class FJInfoType(TypedDict):
    insts: List[PcInst]
    inst_count: int

class PC(TypedDict):
    inst: str

class FJInfo(TypedDict):
    func_name: str
    pc_count: int
    pcs: List[PC]

class WorkMode(Enum):
    inspect = "inspect"
    profile = "profile"
    injection = "injection"

class FJType(Enum):
    none = None
    nop = "nop"
    rev_branch = "rev_branch"
    bitflip1 = "bitflip1"
    bitflip2 = "bitflip2"
    bitflip3 = "bitflip3"
    bitflip_random = "bitflip_random"
    stuck_at_0 = "stuck_at_0"
    stuck_at_1 = "stuck_at_1"

ALL_FJ_TYPES = [
    FJType.nop,
    FJType.bitflip1,
    FJType.bitflip2,
    FJType.bitflip3,
    FJType.bitflip_random,
    FJType.stuck_at_0,
    FJType.stuck_at_1,
]


class RunResult(Enum):
    # Finished without problem
    Success = "Success"

    # int 3 triggered
    SigTrap = "SigTrap"

    # Error detected in cache.cc
    ErrorDetected = "ErrorDetected"

    # the result is mismatch
    TestFailed = "TestFailed"

    # SIGKILL
    Timeout = "Timeout"

    # SIGKILL
    Terminate = "Terminate"

    # SIGSEGV
    SegmentFault = "SegmentFault"

    # FloatPointerError
    FPError = "FPError"

    # FloatPointerError
    UnknownRetcode = "UnknownRetcode"

    # Abort
    Abort = "Abort"

    # unknown error
    Unknown = "Unknown"

    # Ignored
    Ignored = "Ignored"


class RunResultType(TypedDict):
    error: RunResult
    data: Any

class SCEECheckResult(Enum):
    NotExecuted = "NotExecuted"
    SCEE_Detected = "SCEE_Detected"
    SCEE_Detected_NonFatal = "SCEE_Detected_NonFatal"
    SCEE_Not_Detected = "SCEE_Not_Detected"
    NoneSCEE = "NoneSCEE"
    Unknown = "Unknown"

class FJTestResult(TypedDict):
    profile_result: RunResultType
    injection_result: RunResultType
    result: SCEECheckResult

class KOutput(BaseModel):
    retcode: int
    stdout: str
    stderr: str
    
class FaultInfoByUnit(BaseModel):
    fault_type_by_unit: str
    fault_type_by_instr: str
    instr_name: str

class TestBuildDef(BaseModel):
    script: str
    binary: str  # where the build output binary will be stored

class TestRunDef(BaseModel):
    cmd: List[str] = []

class TestDef(BaseModel):
    fj_types: List[FJType]
    build: TestBuildDef
    run: TestRunDef
