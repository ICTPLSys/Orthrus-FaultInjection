import asyncio
import json
import logging
import os
import random
import shutil
from collections import OrderedDict
from multiprocessing.pool import ThreadPool as Pool
from pathlib import Path
from subprocess import PIPE, Popen, TimeoutExpired
from threading import Lock
from typing import Dict, List

import utils.consts as consts
import utils.fj as fj
import utils.parsers as parsers
from utils.helper import log_func, make_uid, timing
from utils.ktypes import *
from utils.testcase import TestCase, parallel_build_fn, parallel_test
from utils.testcase_info import TestcaseInfo
from concurrent.futures import ProcessPoolExecutor
from multiprocessing import Manager, Queue
import time
import signal
from tqdm import tqdm
# from concurrent.futures import ThreadPoolExecutor

logger = logging.getLogger(__name__)


# def input_output_cache(func, cache_vars: List[str]):
#     def wrapper(*args, **kwargs):
#         return func(args, kwargs)
#     return wrapper

'''
    A struct consists of fn_name and pc
'''
class PCInfo:
    def __init__(self, fn_name, pc):
        self.fn_name = fn_name
        self.pc = pc

class Config:
    def __init__(
        self,
        tag: str,
        output: str,

        test_type: str,
        test_mode: str, # lite (fast check) or full
        scee_dir: str,  # the scee folder

        llvm_dir: str,  # llvm_path to locate clang clang++
        temp_dir: str,  # store compiled artifacts
        category: str,
        is_debug: bool = False,
        parallel_build: int = 16,
        parallel_test: int = 64,
        fj_functions: str = "",
        app_name: str = "",
        ft_type: str = "",
    ):
        consts.FLAG_debug = is_debug

        self.output = output

        self.TAG = tag
        self.parallel_build = parallel_build
        self.parallel_test = parallel_test
        self.test_mode = test_mode
        self.fj_functions = fj_functions
        logger.debug(f"parallel build: {self.parallel_build}, test: {self.parallel_test}")

        # src
        self.llvm_dir = Path(llvm_dir).absolute()
        self.scee_dir = Path(scee_dir).absolute()

        # intermediate
        self.temp_dir = Path(temp_dir) / self.TAG
        self.cache_dir = self.temp_dir / "cache"  # cache the important data
        self.build_dir = self.temp_dir / "build"  # build pool
        self.test_dir  = self.temp_dir / "test"   # test binaries

        ## init folders
        os.makedirs(self.temp_dir, exist_ok=True)
        os.makedirs(self.cache_dir, exist_ok=True)
        os.makedirs(self.test_dir, exist_ok=True)
        #if build_dir exists, do not copy again
        logger.info(f"Duplicate {self.parallel_build} build dirs")
        self.build_idx = 0
        self.build_idx_lock = Lock()
        self.build_dirs: List[Tuple[Lock, Path]] = []
        for idx in range(self.parallel_build):
            tmp_build_dir = self.build_dir / f"build_{idx}"
            if not tmp_build_dir.exists():
                shutil.copytree(self.scee_dir, tmp_build_dir, symlinks=True, dirs_exist_ok=True)
            self.build_dirs.append((Lock(), tmp_build_dir))

        # log folders
        logger.info(f"Current Test Tag: {self.TAG}")
        logger.info(f"Current Test Temp Dir: {self.temp_dir}")

        # init test defs
        self.test_type = test_type
        self.test_def = fj.TEST_DEFS[self.test_type]
        self.category = category
        # runtime init vars
        self.cache_func_fj_infos = self.cache_dir / "func_fj_infos.json"
        self.func_fj_infos: Dict[str, FJInfoType]|None = None
        if self.cache_func_fj_infos.exists():
            with open(self.cache_func_fj_infos, "r") as f:
                self.func_fj_infos = json.load(f)

        # store all testcases
        # uid vs TestcaseInfo
        self.app_name = app_name
        self.ft_type = ft_type
        
        self.testcase_infos: OrderedDict[str, TestcaseInfo] = OrderedDict()
        self.fault_info_by_unit_type: Dict[str, int] = {}
        self.predefined_ratio: Dict[str, float] = {
            "fpu": 0.25,
            "alu": 0.25,
            "simd": 0.25,
            "cc": 0.25
        }
        self.each_fault_bias_count: Dict[str, int] = {
            "fpu": 0,
        }

        return

    def __get_temp__(self, tag: str) -> Path:
        tmp = self.temp_dir / tag
        os.makedirs(tmp, exist_ok=True)
        return tmp

    def __get_test__(self, tag: str) -> Path:
        ret = self.test_dir / tag
        os.makedirs(ret, exist_ok=True)
        return ret

    def __get_build__(self) -> Tuple[Lock, Path]:
        with self.build_idx_lock:
            idx = self.build_idx % len(self.build_dirs)
            self.build_idx += 1
        return self.build_dirs[idx]

    def get_setup_env(self) -> Dict[str, str]:
        env = os.environ.copy()
        env["CC"] = self.llvm_dir / "bin" / "clang"
        env["CXX"] = self.llvm_dir / "bin" / "clang++"

        return env

    def run(self):
        # Time each stage
        # inspect and build
        self.stage_0_build_all()
        # self.stage_1_inspect()
        # Decide which insts to inject
        self.stage_2_profile()
        # Build injection binaries
        # self.stage_3_build_injection_artifacts()
        # Run injection
        self.stage_3_injection()
        return


    @log_func
    @timing
    def __stage_0_profile_prepare(self):
        ''' build all profile artifacts '''

        profile_testcases = []
        for fn_name, fn_fj_info in self.func_fj_infos.items():
            logger.debug(f"Build {fn_name} profile binaries")
            total_pcs = fn_fj_info["inst_count"]
            for pc_inst in fn_fj_info["insts"]:
                pc = pc_inst["pc"]
                inst = pc_inst["inst"]

                # Build
                tag = f"profile|{fn_name}|{pc}"
                lock, build_dir = self.__get_build__()
                uid = make_uid(fn_name, pc)
                # TODO: remove the lock
                testcase = TestCase(
                    tag=tag,
                    build_root=build_dir,
                    test_root=self.__get_test__(tag),
                    build_lock=lock,
                    env=self.get_setup_env(),
                    test_def=self.test_def,
                    fj_conf = fj.build_fj_conf(workMode=WorkMode.profile, fj_type=FJType.nop, TargetFunc=fn_name, pc=pc, category=self.category)
                )

                testcase_info = self.testcase_infos.get(uid, TestcaseInfo())
                testcase_info.profile_testcase = testcase
                self.testcase_infos[uid] = testcase_info

                profile_testcases.append(testcase)

    '''
    run() only achieves parallelism in each stage
    run_parallel() will achieve parallelism in each pc, each pc run
    all stages after inspect should be done in pc parallelism
    '''
    @log_func
    @timing
    def run_parallel(self, separate_profile: bool = False):
        def cleanup():
            logger.info("Cleaning up before exiting...")
            mm = {}
            with open(self.output, "w", encoding="utf8") as f:
                for uid, testcase_info in self.testcase_infos.items():
                    if testcase_info._is_finished:
                        mm[uid] = {
                            "profile": str(testcase_info.profile_result["error"]),
                            "injection": [],
                        }
                        for injection_testcase, result in zip(testcase_info.injection_testcases, testcase_info.injection_results):
                            tag = injection_testcase.tag
                            mm[uid]["injection"].append({
                                "name": tag,
                                "result": {
                                    "error": str(result["error"]),
                                    "data": result["data"],
                                },
                                "fj_conf": injection_testcase.fj_conf,
                            })
                
                json.dump(mm, f, indent=2, ensure_ascii=False)

        def signal_handler(signum, frame):
            cleanup()
            raise KeyboardInterrupt

        # profile and injection individually
        
        # build same with run()
        # self.stage_0_build_all()
        self.__stage_0_inspect()
        self.__stage_0_profile_prepare()
        # build a Dict, consists of fn_name and pc
        pc_infos: List[PCInfo] = []
        for fn_name, fn_fj_info in self.func_fj_infos.items():
            for pc_inst in fn_fj_info["insts"]:
                pc = pc_inst["pc"]
                # insert an entry to uid_to_testcase_info
                pc_infos.append(PCInfo(fn_name, pc))
                
        logger.info(f"size of pc_infos: {len(pc_infos)}")
        # profile and inject in parallel
        # with Pool(self.parallel_test) as pool:
        #     pool.map(self.single_pc_profile_and_inject, pc_infos)

        signal.signal(signal.SIGINT, signal_handler)
        # profile and inject in parallel
        if separate_profile:
            if self.test_mode == "lite":
                # randomly select 50 of the pc_infos
                lite_pc_infos = random.sample(pc_infos, 100)
                logger.info(f"randomly selected {len(lite_pc_infos)} pc_infos")
                try:
                    # if 99% of the tasks have finished, then exit
                    with Pool(self.parallel_test) as pool:
                        with tqdm(
                        total=len(lite_pc_infos), 
                        desc="Profiling", 
                        unit="task", 
                        unit_scale=True,
                        mininterval=1,
                            bar_format="{l_bar}{bar} | {n_fmt}/{total_fmt} [{rate_fmt}]",
                        ) as pbar:
                            for _ in pool.imap_unordered(self.single_pc_profile_only, lite_pc_infos):
                                pbar.update(1)
                                if pbar.n > pbar.total - 2:
                                    break
                except KeyboardInterrupt:
                    logger.warning("Process interrupted by user.")
                    return
                
                self.stat_fault_type_by_unit()

                try:
                    with Pool(self.parallel_test) as pool:
                        with tqdm(
                        total=len(lite_pc_infos), 
                        desc="Injecting", 
                        unit="task", 
                        unit_scale=True,
                        mininterval=1,
                            bar_format="{l_bar}{bar} | {n_fmt}/{total_fmt} [{rate_fmt}]",
                        ) as pbar:
                            for _ in pool.imap_unordered(self.single_pc_injection_only, lite_pc_infos):
                                pbar.update(1)
                                if pbar.n > pbar.total - 2:
                                    break
                except KeyboardInterrupt:
                    logger.warning("Process interrupted by user.")
                    return
            else:
                try:
                    # if 99% of the tasks have finished, then exit
                    with Pool(self.parallel_test) as pool:
                        with tqdm(
                        total=len(pc_infos), 
                        desc="Profiling", 
                        unit="task", 
                        unit_scale=True,
                        mininterval=1,
                            bar_format="{l_bar}{bar} | {n_fmt}/{total_fmt} [{rate_fmt}]",
                        ) as pbar:
                            for _ in pool.imap_unordered(self.single_pc_profile_only, pc_infos):
                                pbar.update(1)
                                if pbar.n > 0.995 * pbar.total:
                                    break
                except KeyboardInterrupt:
                    logger.warning("Process interrupted by user.")
                    return
                
                self.stat_fault_type_by_unit()

                try:
                    with Pool(self.parallel_test) as pool:
                        with tqdm(
                        total=len(pc_infos), 
                        desc="Injecting", 
                        unit="task", 
                        unit_scale=True,
                        mininterval=1,
                            bar_format="{l_bar}{bar} | {n_fmt}/{total_fmt} [{rate_fmt}]",
                        ) as pbar:
                            for _ in pool.imap_unordered(self.single_pc_injection_only, pc_infos):
                                pbar.update(1)
                                if pbar.n > 0.99 * pbar.total:
                                    break
                except KeyboardInterrupt:
                    logger.warning("Process interrupted by user.")
                    return
        else:
            try:
                with Pool(self.parallel_test) as pool:
                    with tqdm(
                    total=len(pc_infos), 
                    desc="Processing", 
                    unit="task", 
                    unit_scale=True,
                    mininterval=1,
                        bar_format="{l_bar}{bar} | {n_fmt}/{total_fmt} [{rate_fmt}]",
                    ) as pbar:
                        for _ in pool.imap_unordered(self.single_pc_profile_and_inject, pc_infos):
                            pbar.update(1)
            except KeyboardInterrupt:
                logger.warning("Process interrupted by user.")
                return
            
        cleanup()

    @log_func
    def stat_fault_type_by_unit(self):
        for uid, testcase_info in self.testcase_infos.items():
            if testcase_info._is_profile_hit:
                fault_info_by_unit: FaultInfoByUnit = testcase_info.fault_info_by_unit
                if fault_info_by_unit.fault_type_by_unit not in self.fault_info_by_unit_type:
                    self.fault_info_by_unit_type[fault_info_by_unit.fault_type_by_unit] = 0
                self.fault_info_by_unit_type[fault_info_by_unit.fault_type_by_unit] += 1

        for fault_type, count in self.fault_info_by_unit_type.items():
            logger.info(f"Fault type by individual instructions: {fault_type}, count: {count}")

        # calculate the percentage of each fault type, and calculate their weight to reach the predefined ratio
        # max_count = max(self.fault_info_by_unit_type.values())
        if (self.category == "computational"):
            max_count = 0
            max_count_single = 0
            for fault_type, count in self.fault_info_by_unit_type.items():
                max_count = max(max_count, (int)(count / self.predefined_ratio[fault_type]))
                max_count_single = max(max_count_single, (int)(count))

            for fault_type, count in self.fault_info_by_unit_type.items():
                self.each_fault_bias_count[fault_type] = (int)(max_count * self.predefined_ratio[fault_type] / count * 5)

            logger.info(f"stat max_count: {max_count}")
            logger.info(f"each_fault_bias_count: {self.each_fault_bias_count}")
            logger.info(f"max_count_single: {max_count_single}")
            # save the max_count_single to a file
            # get the home directory
            home_dir = os.getenv("HOME")
            log_path = f"{home_dir}/scee/fw/logs/{self.app_name}_{self.ft_type}_max_count_single.txt"
            os.makedirs(os.path.dirname(log_path), exist_ok=True)
            with open(log_path, "w") as f:
                f.write(str(max_count_single))
        else:
            # read the max_count_single from a file
            home_dir = os.getenv("HOME")
            log_path = f"{home_dir}/scee/fw/logs/{self.app_name}_{self.ft_type}_max_count_single.txt"
            max_count_single = 0
            with open(log_path, "r") as f:
                max_count_single = int(f.read())
            for fault_type, count in self.fault_info_by_unit_type.items():
                self.each_fault_bias_count[fault_type] = (int)(max_count_single / count) * 10
            logger.info(f"each_fault_bias_count: {self.each_fault_bias_count}")
        return

    @log_func
    def single_pc_profile_and_inject(self, data: PCInfo):
        #TODO: move __stage_0_build_profile_artifacts into this function
        fn_name = data.fn_name
        pc = data.pc
        uid = make_uid(fn_name, pc)
        testcase_info_meta = self.testcase_infos[uid]
        # 0. build profile testcase
        logger.debug(f"Testcase({uid}): building profile binaries")
        koutput = testcase_info_meta.profile_testcase.build()
        fault_info_by_unit: FaultInfoByUnit = parsers.get_fault_type_from_build_result(koutput.stdout)
        # if is "", then it is not a fault
        if (fault_info_by_unit.fault_type_by_unit == ""):
            logger.info(f"Testcase({uid}): can not generate fault here, Ignoring")
            return
        
        # strstr = "[FaultInfo]: fpu calc ADDSDrr"
        logger.debug(f"fault_info_by_unit: {fault_info_by_unit}")
        # parse the build output to get injection type.
        logger.debug(f"Testcase({uid}): building profile binaries done")
        # 1. profile
        koutput: KOutput = testcase_info_meta.profile_testcase.test()
        profile_result: RunResultType = parsers.parse_run_result(koutput.retcode, koutput.stdout, koutput.stderr)
        testcase_info_meta.profile_result = profile_result
        # remove the testcase if profile not hit
        if (p_result := testcase_info_meta.profile_result["error"]) != RunResult.SigTrap:
            if p_result == RunResult.Success:
                logger.info(f"Testcase({uid}): profile not hit, Ignoring")
            else:
                logger.error(f"Testcase({uid}): profile Result Strange: {p_result}")
            return
        
        testcase_info_meta._is_profile_hit = True
        testcase_info_meta.fault_info_by_unit = fault_info_by_unit
        logger.info(f"Testcase({uid}): profile hit, building injection binaries")
        
        
        # 2. build inject
        injection_testcases = []
        
        for fj_type in self.test_def.fj_types:
            logger.debug(f"Build {fn_name} injection binaries @ {fj_type}")

            # Build
            tag = f"injection|{fn_name}|{pc}|{fj_type}|{fault_info_by_unit.fault_type_by_unit}|{fault_info_by_unit.fault_type_by_instr}"
            lock, build_dir = self.__get_build__()
            testcase = TestCase(
                tag=tag,
                build_root=build_dir,
                test_root=self.__get_test__(tag),
                build_lock=lock,
                env=self.get_setup_env(),
                test_def=self.test_def,
                fj_conf = fj.build_fj_conf(workMode=WorkMode.injection, fj_type=fj_type, TargetFunc=fn_name, pc=pc, category=self.category)
            )
            testcase_info = self.testcase_infos[uid]
            testcase_info.injection_testcases.append(testcase)
            self.testcase_infos[uid] = testcase_info
            injection_testcases.append(testcase)
        
        for testcase in testcase_info.injection_testcases:
            testcase.build()
    
        logger.debug(f"Testcase({uid}): building injection binaries done")
            
        # 3. run injected binaries
        for testcase in testcase_info.injection_testcases:
            koutput: KOutput = testcase.test()
            injection_result: RunResultType = parsers.parse_run_result(koutput.retcode, koutput.stdout, koutput.stderr)
            testcase_info.injection_results.append(injection_result)
        
        testcase_info._is_finished = True
        logger.debug(f"Testcase({uid}): injection done")
        return
    
    @log_func
    def single_pc_profile_only(self, data: PCInfo):
        fn_name = data.fn_name
        pc = data.pc
        uid = make_uid(fn_name, pc)
        testcase_info_meta = self.testcase_infos[uid]
        # 0. build profile testcase
        logger.debug(f"Testcase({uid}): building profile binaries")
        koutput = testcase_info_meta.profile_testcase.build()
        fault_info_by_unit: FaultInfoByUnit = parsers.get_fault_type_from_build_result(koutput.stdout)
        # if is "", then it is not a fault
        if (fault_info_by_unit.fault_type_by_unit == ""):
            logger.info(f"Testcase({uid}): can not generate fault here, Ignoring")
            return
        
        # strstr = "[FaultInfo]: fpu calc ADDSDrr"
        logger.debug(f"fault_info_by_unit: {fault_info_by_unit}")
        # parse the build output to get injection type.
        logger.debug(f"Testcase({uid}): building profile binaries done")
        # 1. profile
        koutput: KOutput = testcase_info_meta.profile_testcase.test()
        profile_result: RunResultType = parsers.parse_run_result(koutput.retcode, koutput.stdout, koutput.stderr)
        testcase_info_meta.profile_result = profile_result
        # remove the testcase if profile not hit
        if (p_result := testcase_info_meta.profile_result["error"]) != RunResult.SigTrap:
            if p_result == RunResult.Success:
                logger.info(f"Testcase({uid}): profile not hit, Ignoring")
            else:
                logger.error(f"Testcase({uid}): profile Result Strange: {p_result}")
            return
        
        testcase_info_meta._is_profile_hit = True
        testcase_info_meta.fault_info_by_unit = fault_info_by_unit
        logger.info(f"Testcase({uid}): profile hit, done")
        return
    
    @log_func
    def single_pc_injection_only(self, data: PCInfo):
        fn_name = data.fn_name
        pc = data.pc
        uid = make_uid(fn_name, pc)
        if not self.testcase_infos[uid]._is_profile_hit:
            logger.info(f"Testcase({uid}): profile not hit, Ignoring")
            return
        
        testcase_info_meta = self.testcase_infos[uid]
        fault_info_by_unit: FaultInfoByUnit = testcase_info_meta.fault_info_by_unit
        

        injection_testcases = []
        
        if self.each_fault_bias_count[fault_info_by_unit.fault_type_by_unit] > len(self.test_def.fj_types):
            # for the pre self.test_def.fj_types, we do as normal
            processed_count = 0
            for fj_type in self.test_def.fj_types:
                logger.debug(f"Build {fn_name} injection binaries @ {fj_type}")

                # Build
                tag = f"injection|{fn_name}|{pc}|{fj_type}|{fault_info_by_unit.fault_type_by_unit}|{fault_info_by_unit.fault_type_by_instr}"
                lock, build_dir = self.__get_build__()
                testcase = TestCase(
                    tag=tag,
                    build_root=build_dir,
                    test_root=self.__get_test__(tag),
                    build_lock=lock,
                    env=self.get_setup_env(),
                    test_def=self.test_def,
                    fj_conf = fj.build_fj_conf(workMode=WorkMode.injection, fj_type=fj_type, TargetFunc=fn_name, pc=pc, category=self.category)
                )
                testcase_info = self.testcase_infos[uid]
                testcase_info.injection_testcases.append(testcase)
                self.testcase_infos[uid] = testcase_info
                injection_testcases.append(testcase)
                processed_count += 1
                if self.test_mode == "lite":
                    break

            # for the rest, we do as a random bitflip
            for ii in range((int)(self.each_fault_bias_count[fault_info_by_unit.fault_type_by_unit] - processed_count)):
                if self.test_mode == "lite":
                    break
                fj_type = FJType.bitflip_random
                tag = f"injection|{fn_name}|{pc}|{fj_type}|{ii}|{fault_info_by_unit.fault_type_by_unit}|{fault_info_by_unit.fault_type_by_instr}"
                lock, build_dir = self.__get_build__()
                testcase = TestCase(
                    tag=tag,
                    build_root=build_dir,
                    test_root=self.__get_test__(tag),
                    build_lock=lock,
                    env=self.get_setup_env(),
                    test_def=self.test_def,
                    fj_conf = fj.build_fj_conf(workMode=WorkMode.injection, fj_type=fj_type, TargetFunc=fn_name, pc=pc, category=self.category)
                )
                testcase_info = self.testcase_infos[uid]
                testcase_info.injection_testcases.append(testcase)
                self.testcase_infos[uid] = testcase_info
                injection_testcases.append(testcase)
                processed_count += 1
        else:
            processed_count = 0
            for fj_type in self.test_def.fj_types:
                logger.debug(f"Build {fn_name} injection binaries @ {fj_type}")

                # Build
                tag = f"injection|{fn_name}|{pc}|{fj_type}|{fault_info_by_unit.fault_type_by_unit}|{fault_info_by_unit.fault_type_by_instr}"
                lock, build_dir = self.__get_build__()
                testcase = TestCase(
                    tag=tag,
                    build_root=build_dir,
                    test_root=self.__get_test__(tag),
                    build_lock=lock,
                    env=self.get_setup_env(),
                    test_def=self.test_def,
                    fj_conf = fj.build_fj_conf(workMode=WorkMode.injection, fj_type=fj_type, TargetFunc=fn_name, pc=pc, category=self.category)
                )
                testcase_info = self.testcase_infos[uid]
                testcase_info.injection_testcases.append(testcase)
                self.testcase_infos[uid] = testcase_info
                injection_testcases.append(testcase)
                processed_count += 1
                if self.test_mode == "lite":
                    break
                if processed_count > self.each_fault_bias_count[fault_info_by_unit.fault_type_by_unit]:
                    break
        
        for testcase in testcase_info.injection_testcases:
            testcase.build()
    
        logger.debug(f"Testcase({uid}): building injection binaries done")
            
        # 3. run injected binaries
        for testcase in testcase_info.injection_testcases:
            koutput: KOutput = testcase.test()
            injection_result: RunResultType = parsers.parse_run_result(koutput.retcode, koutput.stdout, koutput.stderr)
            testcase_info.injection_results.append(injection_result)
        
        testcase_info._is_finished = True
        logger.debug(f"Testcase({uid}): injection done")
        return

    def stage2_worker(data: Tuple[str, TestcaseInfo]):
        uid, testcase_info = data
        koutput: KOutput = testcase_info.profile_testcase.test()
        profile_result: RunResultType = parsers.parse_run_result(koutput.retcode, koutput.stdout, koutput.stderr)
        testcase_info.profile_result = profile_result
        return

    def stage3_worker(data: Tuple[str, TestcaseInfo]):
        uid, testcase_info = data
        if testcase_info.profile_result["error"] != RunResult.SigTrap:
            logger.info(f"Ignore {uid}, not int3 triggered")
            return

        for testcase in testcase_info.injection_testcases:
            koutput: KOutput = testcase.test()
            injection_result: RunResultType = parsers.parse_run_result(koutput.retcode, koutput.stdout, koutput.stderr)
            testcase_info.injection_results.append(injection_result)
        return
    
    @log_func
    @timing
    def stage_0_build_all(self):
        # time these two stages
        self.__stage_0_inspect()
        self.__stage_0_build_profile_artifacts()
        return

    @log_func
    @timing
    def __stage_0_inspect(self):
        '''
            stage_0_{inspect, profile, injection}
            Build all binaries for: inspect, profile, injection
                inspect: int3
                profile:
                injection: all FI types
        '''
        if self.func_fj_infos is not None:
            logger.info("Using cached func_fj_infos for stage_0_build_all")
        else:
            # Build inspect binary
            testcase_tag = "inspect"
            lock, build_dir = self.__get_build__()
            exec_dir = self.__get_test__(testcase_tag)

            # init a new testcase
            #TODO: remove the lock
            testcase = TestCase(
                tag=testcase_tag,
                build_root=build_dir,
                test_root=exec_dir,
                build_lock=lock,
                env=self.get_setup_env(),
                test_def=self.test_def,
                fj_conf = fj.build_fj_conf(workMode=WorkMode.inspect, category=self.category)
            )
            logger.info("Inspecting...")
            
            # build_output = testcase.build()
            build_output = testcase.build_new()

            # parse build_output, then we will get the PC info of each function
            self.func_fj_infos: Dict[str, FJInfoType] = parsers.parse_inspect_build_result(build_output)
            # cache the inspect result to in case crash
            logger.debug(f"func_fj_infos: {self.func_fj_infos}")
            with open(self.cache_func_fj_infos, "w") as f:
                json.dump(self.func_fj_infos, f, indent=4, ensure_ascii=False)

        total_insts = 0
        for fn_name, fj_info in self.func_fj_infos.items():
            inst_count = fj_info["inst_count"]
            logger.info(f"Func({fn_name}), pcs: {inst_count}")
            total_insts += inst_count
        logger.info(f"Total pcs: {total_insts}")
        return

    @log_func
    @timing
    def __stage_0_build_profile_artifacts(self):
        ''' build all profile artifacts '''

        profile_testcases = []
        for fn_name, fn_fj_info in self.func_fj_infos.items():
            logger.debug(f"Build {fn_name} profile binaries")
            total_pcs = fn_fj_info["inst_count"]
            for pc_inst in fn_fj_info["insts"]:
                pc = pc_inst["pc"]
                inst = pc_inst["inst"]

                # Build
                tag = f"profile|{fn_name}|{pc}"
                lock, build_dir = self.__get_build__()
                uid = make_uid(fn_name, pc)
                # TODO: remove the lock
                testcase = TestCase(
                    tag=tag,
                    build_root=build_dir,
                    test_root=self.__get_test__(tag),
                    build_lock=lock,
                    env=self.get_setup_env(),
                    test_def=self.test_def,
                    fj_conf = fj.build_fj_conf(workMode=WorkMode.profile, fj_type=FJType.nop, TargetFunc=fn_name, pc=pc, category=self.category)
                )

                testcase_info = self.testcase_infos.get(uid, TestcaseInfo())
                testcase_info.profile_testcase = testcase
                self.testcase_infos[uid] = testcase_info

                profile_testcases.append(testcase)

        # parallel build
        # print(f"size of profile_testcases: {len(profile_testcases)}")
        logger.info(f"size of profile_testcases: {len(profile_testcases)}")
        with Pool(self.parallel_build) as pool:
            pool.map(parallel_build_fn, profile_testcases)
        # TODO(kuriko): only run triggered cases
        return

    @log_func
    @timing
    def stage_3_build_injection_artifacts(self):
        injection_testcases = []

        for fn_name, fn_fj_info in self.func_fj_infos.items():
                total_pcs = fn_fj_info["inst_count"]
                for pc_inst in fn_fj_info["insts"]:
                    pc = pc_inst["pc"]
                    inst = pc_inst["inst"]

                    uid = make_uid(fn_name, pc)
                    # logger.debug(f"Testcase uid is {uid}")
                    testcase_info = self.testcase_infos[uid]
                    if (p_result := testcase_info.profile_result["error"]) != RunResult.SigTrap:
                        if p_result == RunResult.Success:
                            logger.info(f"Testcase({uid}): profile not hit, Ignoring")
                        else:
                            logger.error(f"Testcase({uid}): profile Result Strange: {p_result}")
                        continue

                    for fj_type in self.test_def.fj_types:
                        logger.debug(f"Build {fn_name} injection binaries @ {fj_type}")

                        # Build
                        tag = f"injection|{fn_name}|{pc}|{fj_type}"
                        lock, build_dir = self.__get_build__()
                        # TODO: remove the lock
                        testcase = TestCase(
                            tag=tag,
                            build_root=build_dir,
                            test_root=self.__get_test__(tag),
                            build_lock=lock,
                            env=self.get_setup_env(),
                            test_def=self.test_def,
                            fj_conf = fj.build_fj_conf(workMode=WorkMode.injection, fj_type=fj_type, TargetFunc=fn_name, pc=pc, category=self.category)
                        )
                        # NOTE(kuriko): it should already inited in profile phase!
                        testcase_info = self.testcase_infos[uid]
                        testcase_info.injection_testcases.append(testcase)
                        self.testcase_infos[uid] = testcase_info
                        injection_testcases.append(testcase)

        # parallel build
        logger.debug(f"size of injection_testcases: {len(injection_testcases)}")
        logger.debug(f"self.parallel_build: {self.parallel_build}")
        
        # Timer this pool period time
        start_time = time.time()
        with Pool(self.parallel_build) as pool:
            pool.map(parallel_build_fn, injection_testcases)
        # with ProcessPoolExecutor(max_workers=self.parallel_build) as executor:
        #     futures = [executor.submit(parallel_build_fn, case) for case in injection_testcases]
            # for idx, future in enumerate(futures):
            #     updated_case = future.result()  
                # injection_testcases[idx] = updated_case
                # logger.debug(f"case._is_build: {updated_case._is_build}")
                
        end_time = time.time()
        print(f"Time taken for parallel build: {end_time - start_time} seconds")
        
        for case in injection_testcases:
            case._is_build = True
        return

    @log_func
    @timing
    def stage_1_inspect(self):
        '''
            build the programe without any modifications
            run the program in inspect mode
            parse stdout to get all function pc info
        '''

        # bypass, done in phase_0
        return
    
    @log_func
    @timing
    def stage_2_profile(self):
        '''
            parse the profile execution and decide this inst run or not
        '''

        def worker(data: Tuple[str, TestcaseInfo]):
            uid, testcase_info = data
            koutput: KOutput = testcase_info.profile_testcase.test()
            profile_result: RunResultType = parsers.parse_run_result(koutput.retcode, koutput.stdout, koutput.stderr)
            testcase_info.profile_result = profile_result
            return

        logger.debug(f"size of profile testcases: {len(self.testcase_infos)}")
        with Pool(self.parallel_test) as pool:
            pool.map(worker, self.testcase_infos.items())
        
        # with ProcessPoolExecutor(max_workers=self.parallel_test) as executor:
        #     futures = [executor.submit(self.stage2_worker, data) for data in self.testcase_infos.items()]
        #     for idx, future in enumerate(futures):
        #         updated_data = future.result()
        #         # self.testcase_infos.items()[idx] = updated_data
        #         # logger.debug(f"testcase_info.profile_result: {updated_data.profile_result}")

        logger.info("Here's the profile results:")
        for idx, (uid, testcase_info) in enumerate(self.testcase_infos.items()):
            logger.info(f'{idx: 4}: {uid}: {testcase_info.profile_result["error"]}')

        return

    @log_func
    @timing
    def stage_3_injection(self):
        def worker(data: Tuple[str, TestcaseInfo]):
            uid, testcase_info = data
            if testcase_info.profile_result["error"] != RunResult.SigTrap:
                logger.info(f"Ignore {uid}, not int3 triggered")
                return

            for testcase in testcase_info.injection_testcases:
                koutput: KOutput = testcase.test()
                injection_result: RunResultType = parsers.parse_run_result(koutput.retcode, koutput.stdout, koutput.stderr)
                testcase_info.injection_results.append(injection_result)

            # TODO(kuriko): log the partial results here

            return

        # NOTE(kuriko): parallel in testcase, not in
        self.stage_3_build_injection_artifacts()
        
        
        with Pool(self.parallel_test) as pool:
            pool.map(worker, self.testcase_infos.items())

        logger.info("Here's the injection results:")
        for idx, (uid, testcase_info) in enumerate(self.testcase_infos.items()):
            if testcase_info.profile_result["error"] == RunResult.SigTrap:
                logger.info(f"{idx: 4}: {uid}: \ninjection_results: {[result['error'] for result in testcase_info.injection_results]}")

        with open(self.output, "w", encoding="utf8") as f:
            mm = {}
            for uid, testcase_info in self.testcase_infos.items():
                mm[uid] = {
                    "profile": str(testcase_info.profile_result["error"]),
                    "injection": [],
                }
                for injection_testcase, result in zip(testcase_info.injection_testcases, testcase_info.injection_results):
                    tag = injection_testcase.tag
                    mm[uid]["injection"].append({
                        "name": tag,
                        "result": {
                            "error": str(result["error"]),
                            "data": result["data"],
                        },
                        "fj_conf": injection_testcase.fj_conf,
                    })

            json.dump(mm ,f, indent=2, ensure_ascii=False)

        return

    @log_func
    def stage_4_mask(self):
        return
