import sys
import os
import re
import json
import fcntl
import shutil
import tempfile
import traceback
from typing import *
from pathlib import Path
from functools import partial as _p
import multiprocessing
from multiprocessing import Pool, Queue
from subprocess import PIPE, Popen
from copy import deepcopy
import logging
from pprint import pformat, pprint
import itertools as itools
from subprocess import TimeoutExpired
from tempfile import TemporaryDirectory
from tqdm import tqdm

from utils.ktypes import *

logger = logging.getLogger(__name__)

template = {
    "workmode": None,
    "profiles": [{
        "name": "batch_processing",
        "match_mode": "normal",
        "function": None,
        "select_mode": "all",
        "faults": [
            {
                "repeat_count": 1,
                "types": "bitflip",
                "pc": [],
                "options": [1, 1],
                "insts_only": [],
                "insts_ignore": []
            }
        ]
    }],
    "insts_only": [
    ],
    "insts_ignore": [
    ]
}

MAX_TIMEOUT = 300

# Must be full match since we use pat.match()
FJ_FUNCTIONS = [
    # re.compile(".*CNN.*"),
    # re.compile(".*_hash_func.*"),
    # re.compile("_ZN8cc_btree11PageManager9AllocPageEv"),
    # re.compile("_ZN8cc_btree15ConcurrentBTree6InsertEmPKvj"),
    # re.compile(".*((_hashmap_delete)|(cache_get)|(cache_set)).*")
    # re.compile(".*((_hashmap_delete)|(cache_get)|(cache_set)).*")
    # re.compile(r"cache_set"),
    # re.compile(r".*_check.*"),
    # re.compile(r"hashmap_hash_main"),
    # re.compile(r"hashmap_get"),
]

IS_OUT = False

IGNORED_FJ_FUNCTIONS = [
    # re.compile(r".*BuckLoad.*", re.IGNORECASE),
    # re.compile(r".*test.*", re.IGNORECASE),
    # re.compile(r".*OperationTest_Test.*"),
    # re.compile(r".*_GLOBAL_.*"),
    # re.compile(r".*clang.*"),
    # re.compile(r".*uniform_int.*"),
    # re.compile(r".*mersenne_twister.*"),
    re.compile(r"main"),

    # only appliable for LeNet-5
    # re.compile(r".*UpdateWeights.*", re.IGNORECASE),
    # re.compile(r".*update_weights_bias.*", re.IGNORECASE),
    # re.compile(r".*ModelFile.*", re.IGNORECASE),
    # re.compile(r".*_check.*"),
    # re.compile(r"hashmap_hash"),
    # re.compile(r"hashmap_get"),
]

class Config:
    def __init__(self,
                test_exec: Path,
                scee_dir: Optional[Path],
                temp_dir: Path,
                llvm_dir: Path,
                threads: Optional[int] = None,
                fj_conf_file: Optional[Path] = None,
                verbose: bool = False,
        ):
        self.verbose = verbose

        temp_dir = temp_dir.absolute()
        llvm_dir = llvm_dir.absolute()

        self.test_exec = test_exec
        if scee_dir:
            self.set_scee_dir(scee_dir, self.test_exec)

        self.threads = threads
        if self.threads:
            self.scee_pool = [None for _ in range(self.threads)]

        # self.fj_conf_dir: Path = temp_dir / "ktests"
        # self.fj_conf_dir.mkdir(parents=True, exist_ok=True)

        # delete temp dir when class destructed
        self.__temp_dir_holder = tempfile.TemporaryDirectory(prefix="ktest_", dir=temp_dir)
        self.temp_dir = Path(self.__temp_dir_holder.name)
        if fj_conf_file == None:
            self.fj_conf_file = self.temp_dir / "config.json"
        else:
            self.fj_conf_file = Path(fj_conf_file).absolute()

        self.env = os.environ.copy()
        self.llvm_dir = llvm_dir
        self.env['CC'] = self.llvm_dir / "clang"
        self.env['CXX'] = self.llvm_dir / "clang++"

    def set_fj_conf_dir(self, fj_conf_dir: Path):
        self.fj_conf_dir = fj_conf_dir
        self.fj_conf_file = self.fj_conf_dir / "config.json"
        return

    def set_scee_dir(self, scee_dir: Path, test_exec: Path):
        self.scee_dir: Path = scee_dir
        self.scee_build_dir: Path = self.scee_dir / "build"
        self.scee_build_dir.mkdir(parents=True, exist_ok=True)
        self.test_exec_path = self.scee_build_dir / "tests" / test_exec
        return


# Utils

def write_fj_conf(filepath: Path, config: dict):
    with open(filepath, "w") as f:
        json.dump(config, f, ensure_ascii=False, indent=2)
    return

def build_test(cfg: Config, is_stdout=False, is_fj = True, env: Dict[str, str] = {}, uid=-1) -> Tuple[str, str]:
    tmp_env = deepcopy(cfg.env)
    tmp_env['KDEBUG'] = "ON"
    if is_fj:
        FJ_CONFIG_FILE = cfg.fj_conf_file
        tmp_env['FJ_CONFIG'] = FJ_CONFIG_FILE
    # load env from parameters
    tmp_env.update(env)

    # copy build.sh
    # FIXME(kuriko): assume that env.sh & build.sh besides to .py file
    shutil.copy("env.sh", cfg.scee_build_dir)
    shutil.copy("build.sh", cfg.scee_build_dir)
    # logger.debug(f"Build Env: \n{pformat(tmp_env)}")
    logger.debug(f"cwd: {cfg.scee_dir}")
    if is_fj:
        logger.debug(f"FJ_CONF: {FJ_CONFIG_FILE}")
    if is_stdout:
        kout = sys.stdout
        kerr = sys.stderr
    else:
        kout = PIPE
        kerr = PIPE

    logger.debug("running build.sh")
    # p = Popen(['./build.sh'], env=tmp_env, cwd=cfg.scee_build_dir)
    # input("continue?")
    cmd = ['./build.sh', str(uid)]
    p = Popen(cmd, env=tmp_env, cwd=cfg.scee_build_dir, stdout=kout, stderr=kerr)
    try:
        out, err = p.communicate(timeout=MAX_TIMEOUT)

        out = out.decode("utf-8")
        err = err.decode("utf-8")

        rc = p.returncode
        if rc != 0:
            print("!!!!!! Build Error !!!!!!")
            print("out: ", out)
            print("err: ", err)
            raise Exception("Process exit non ZERO")
    except Exception as e:
        logger.error(e)
        print(traceback.format_exc())
        raise e

    if cfg.verbose:
        print(f"verbose: {cfg.verbose}")
        logger.debug("-"*40)
        logger.debug("Build Log:")
        logger.debug(f"out: {out}")
        logger.debug(f"err: {out}")
        logger.debug("-"*40)
    return out, err

def prepare_build_pool_worker(i, cfg):
    logger.info(f"Prepare pool#{i}")
    temp_scee_dir: Path = cfg.temp_dir / f"scee{i}"
    temp_scee_build_dir: Path = temp_scee_dir / f"build"

    shutil.copytree(cfg.scee_dir, temp_scee_dir, symlinks=True)

    shutil.rmtree(temp_scee_build_dir)
    temp_scee_build_dir.mkdir(parents=True, exist_ok=True)

    _cfg = Config(
        test_exec = cfg.test_exec,
        scee_dir = temp_scee_dir,
        temp_dir = cfg.temp_dir,
        llvm_dir= cfg.llvm_dir,
        verbose = cfg.verbose,
        threads = None,
    )
    build_test(_cfg, is_fj=False, uid=i)

    return temp_scee_dir

def prepare_build_pool(cfg: Config):
    with Pool() as pool:
        cfg.scee_pool = pool.map(_p(prepare_build_pool_worker, cfg=cfg), range(cfg.threads))

    pool.close()
    pool.join()

    logger.info("Prepare Finished")

    return

def run_operation_test(cfg: Config, uid):
    # p = Popen(["tests/operation/operation_test"], cwd=cfg.scee_build_dir, env=cfg.env, stdout=PIPE, stderr=PIPE)
    logger.debug("running test_exec")
    cfg.env["UNIQUE_RUN_ID"] = str(uid)
    p = Popen([cfg.test_exec_path], cwd=cfg.scee_build_dir, env=cfg.env, stdout=PIPE, stderr=PIPE)
    try:
        out, err = p.communicate(timeout=MAX_TIMEOUT)
        out = out.decode("utf-8")
        err = err.decode("utf-8")
        rc = p.returncode
    except TimeoutExpired as e:
        out = ""
        err = ""
        rc = -4
    p.kill()

    return rc, out, err

# Stages

def build_cfg_from_existing(cfg: Config) -> Config:
    # temp_dir = tempfile.TemporaryDirectory(prefix="ktest_", dir=cfg.temp_dir, delete=False)
    temp_dir = tempfile.mkdtemp(prefix="ktest_", dir=cfg.temp_dir)
    temp_dir_path = Path(temp_dir)

    scee_dir = temp_dir_path / "scee"
    shutil.copytree(cfg.scee_dir, scee_dir, dirs_exist_ok=True, symlinks=True)

    _cfg = Config (
        test_exec = cfg.test_exec,
        scee_dir = scee_dir,
        temp_dir = temp_dir,
        llvm_dir = cfg.llvm_dir,
        verbose = cfg.verbose,
    )

    return _cfg

def stage_1_inspect(cfg: Config) -> Dict[str, FnInstInfoLarge]:
    logger.info(f"Stage 1: Inspect all")
    # _cfg = build_cfg_from_existing(cfg)
    _cfg = cfg

    conf = build_fj_conf(WorkMode.inspect, None, None)
    if cfg.verbose:
        logger.debug(f"FJ Config: {conf}")
        # input("Check fj config finished?")
    write_fj_conf(_cfg.fj_conf_file, conf)
    out, _ = build_test(_cfg)

    # get function and instruction info, especially the insts count
    f_info = parse_inspect_build_result(out)

    return f_info


def stage_2_profile_worker(prog, cfg: Config, target_f: str, target_pc: int, inst: str):
    logger.info(f"Progress: {prog}")
    try:
        pool_id = multiprocessing.current_process()._identity[0] % cfg.threads
    except Exception as e:
        pool_id = 0

    logger.info(f"using pool#{pool_id}")
    logger.info(f"Stage 2: Profile on PC {target_f} @ {target_pc}")

    try:
        # change cfg according to the pool id
        cfg = Config(
            test_exec = cfg.test_exec,
            scee_dir=cfg.scee_pool[pool_id],
            temp_dir=cfg.temp_dir,
            llvm_dir=cfg.llvm_dir,
            verbose=cfg.verbose,
        )

        lock_file = cfg.scee_build_dir / "lock"
        logger.debug(f"Locking on {lock_file}")
        lock = open(lock_file, "w+")
        fcntl.flock(lock, fcntl.LOCK_EX)

        conf = build_fj_conf(WorkMode.profile, target_f, target_pc)
        write_fj_conf(cfg.fj_conf_file, conf)


        logger.info(f"Stage 2: Profile on PC {target_f} @ {target_pc}")
        build_out, _ = build_test(cfg)

        fj_insts = parse_profile_build_result(build_out)
        if len(fj_insts) == 0:
            logger.error("No fj_insts found")
            fcntl.flock(lock, fcntl.LOCK_UN)
            return None

        logger.info(f"Current FJ: \n{fj_insts}")

        # NOTE(kuriko): since a `.o` can be linked many times, we fj_insts may have many same fj_info
        #   we currently only check the first entry.
        assert fj_insts[0]["pc"]  == target_pc, f"Mismatch fj pc {fj_insts[0]['pc']} != {target_pc}"

        # build success
        ## TODO(kuriko): copy file to a new location and parallel the build ?
        ## Divide the part 2 into a seperate folder so that we can parallel the tests.

        ## run profile
        rc, out, err = run_operation_test(cfg, pool_id)
        profile_run_result = parse_run_result(rc, out, err)
        logger.debug(profile_run_result)

        # run injection test
        build_out2, rc2, out2, err2 = stage_3_injection_worker(cfg, target_f, target_pc, pool_id)
        injection_run_result = parse_run_result(rc2, out2, err2)
        logger.debug(injection_run_result)

        # run mask test
        # build_out3, rc3, out3, err3 = stage_4_mask_test_worker(cfg, target_f, target_pc)
        # mask_run_result = parse_run_result(rc3, out3, err3)
        mask_run_result = {
            'error': RunResult.Ignored,
            'data': None,
        }
        logger.debug(mask_run_result)

        final_result = parse_profile_injection_result(profile_run_result, injection_run_result, mask_run_result)

        logger.debug(final_result)


        ret = {
            "pc": target_pc,
            "function": target_f,
            "inst": inst,
            "profile_build": build_out.splitlines() if IS_OUT else [],
            "injection_build": build_out2.splitlines() if IS_OUT else [],
            # "mask_build": build_out3.splitlines(),
            "result": {
                "profile": profile_run_result,
                "injection": injection_run_result,
                "mask": mask_run_result,
                "final": final_result,
            }
        }
        pprint(ret)
        # logger.info(ret['result']['profile']['error'])
        # logger.info(ret['result']['injection']['error'])
        # logger.info(ret['result']['final'])

    finally:
        fcntl.flock(lock, fcntl.LOCK_UN)

    return ret

def stage_2_profile(cfg: Config, f_info: Dict[str, FnInstInfoLarge]):
    prepare_build_pool(cfg)

    pool = Pool(processes=cfg.threads)

    cnt = 0

    temp = {}
    final = {}

    # pprint(f_info)
    # input("continue f_info?")

    idx = 0
    tot = 0
    for fname, info in f_info.items():
        Found = False
        for pat in FJ_FUNCTIONS:
            if pat.match(fname):
                Found = True
                break
        if not Found: continue
        for obj in info["insts"]:
            tot += 1

    for fname, info in f_info.items():
        temp[fname] = []
        final[fname] = []

        Found = False
        # First check include list
        for pat in FJ_FUNCTIONS:
            if pat.match(fname):
                Found = True
                break
        if not Found: continue

        logger.info(f"FJ on function: {fname}, total insts: {info['inst_count']}")

        for obj in info["insts"]:
            pc = obj['pc']
            inst = obj['inst']

            # if pc != 20: continue
            # prepare prequisition
            # _cfg = build_cfg_from_existing(cfg)

            cnt += 1
            try:
                print(obj)
                ret = pool.apply_async(stage_2_profile_worker, args=((idx, tot), cfg, fname, pc, inst))
                idx += 1
                # ret = stage_2_profile_worker(cfg, fname, pc, inst)
                # if not ret:
                #     continue
                temp[fname].append(ret)
            except Exception as e:
                logger.error(traceback.format_exc())
                logger.error(f"Exception: {e}")
                continue

            # FIXME(kuriko): this is only for testing.
            # break

    # ret = temp

    pool.close()
    pool.join()

    for fname, data in temp.items():
        for ret in data:
            try:
                final[fname].append(ret.get())
            except Exception as e:
                logger.error(e)
                logger.error(traceback.format_exc())
                logger.error("Process stage2 worker exited returning non ZERO")
                continue


    return final

def stage_3_injection_worker(cfg: Config, target_f: str, target_pc: int, uid):
    logger.info(f"Stage 3: Injection on PC {target_f} @ {target_pc}")
    ## run injection
    conf = build_fj_conf(WorkMode.injection, target_f, target_pc)
    write_fj_conf(cfg.fj_conf_file, conf)
    build_out, _ = build_test(cfg, is_stdout=False)

    logger.debug(f"stage 3 {target_f} @ {target_pc} build finished")

    rc, out, err = run_operation_test(cfg, uid)

    logger.debug(f"stage 3 {target_f} @ {target_pc} test run finished")

    return build_out, rc, out, err

def stage_4_mask_test_worker(cfg: Config, target_f: str, target_pc: int):
    '''
        rebuild the test with recheck disabled
    '''
    logger.info(f"Stage 4: MaskTest on PC {target_f} @ {target_pc}")
    conf = build_fj_conf(WorkMode.injection, target_f, target_pc)
    write_fj_conf(cfg.fj_conf_file, conf)

    build_out, _ = build_test(cfg, env = {"KTEST_NON_SAFE": "ON"})

    logger.debug(f"stage 4 {target_f} @ {target_pc} build finished")

    rc, out, err = run_operation_test(cfg)

    logger.debug(f"stage 4 {target_f} @ {target_pc} test run finished")

    return build_out, rc, out, err

def run_test(cfg: Config):
    f_info = stage_1_inspect(cfg)
    ret = stage_2_profile(cfg, f_info)

    with open("result.json", "w") as f:
        json.dump(ret, f, indent=2, ensure_ascii=False)

    print("dump finished")
    # pprint(ret)

    return ret

# Parser
def parse_profile_build_result(ss: str) -> List[FnInstInfoLarge]:
    # [FaultInject] Inst.0: frame-setup PUSH64r killed $r15, implicit-def $rsp, implicit $rsp
    pat1 = re.compile(r"\[FaultInject\]\s+Inst\.(\d+): (.+)")

    fj_insts: List[FnInstInfoLarge] = []
    for line in ss.splitlines():
        if (matches := pat1.match(line)):
            fj_insts.append({
                "pc": int(matches.group(1)),
                "inst": matches.group(2),
            })

    return fj_insts

def parse_inspect_build_result(ss: str) -> Dict[str, FnInstInfoLarge]:
    # Current Machine Function: _Z3bari  (Real: bar(int))
    pat1 = re.compile(r"^Current Machine Function: (.+)\s+\[Real: (.+)\]$")
    pat2 = re.compile(r"^Current Machine Function: (.+)$")

    # [Inst.34]: $rcx = MOV64rr $rbp
    pat_inst = re.compile(r"\[Inst\.(\d+)\]: (.+)")

    f_info = {}

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
            f_info[func_name]: FnInstInfoLarge = {
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

    for line in itools.chain(out, err):
        # $PATH/tests/operation/operation_test.cc:26: Failure
        test_failed_pat = re.compile(r"(.+\..+:\d+): Failure")
        if (matches := test_failed_pat.match(line)):
            ret['error'] = RunResult.TestFailed
            ret['data'] = matches.group(0)
            return ret

        #  [$PATH/lib/cache.cc:47] Assertion failed:false
        detected_pat = re.compile(r"\[(.+\..+:\d+)\] Assertion failed")
        if (matches := detected_pat.match(line)):
            ret['error'] = RunResult.ErrorDetected
            ret['data'] = {
                "log": matches.group(0),
                "file": matches.group(1),
            }
            return ret

    # something wrong
    logger.debug(rc)
    logger.debug(out)
    logger.debug(err)
    # input("Unknown Error, Continue?")

    return ret

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


def build_fj_conf(workMode: WorkMode, TargetFunc: Optional[str] = None, pc: Optional[int] = None):
    tmp = deepcopy(template)
    tmp["workmode"] = workMode.name

    if workMode == WorkMode.inspect:
        tmp["profiles"][0]["function"] = "*"
    elif workMode == WorkMode.profile:
        pass
    elif workMode == WorkMode.injection:
        pass

    if TargetFunc != None:  # FJ on specific function
        tmp["profiles"][0]["function"] = TargetFunc
    else:
        assert workMode == WorkMode.inspect, "No Function name in non-inspect mode."

    if pc != None:
        tmp["profiles"][0]["faults"][0]["pc"] = [pc, ]

    return tmp
