from dataclasses import dataclass, field
from typing import List

from utils.ktypes import RunResultType, SCEECheckResult, FaultInfoByUnit
from utils.testcase import TestCase


@dataclass
class TestcaseInfo:
    _is_finished: bool = False
    _is_profile_hit: bool = False
    profile_testcase: TestCase|None = None
    profile_result: RunResultType|None = None
    fault_info_by_unit: FaultInfoByUnit|None = None

    injection_testcases: List[TestCase] = field(default_factory=list)
    injection_results: List[RunResultType] = field(default_factory=list)
