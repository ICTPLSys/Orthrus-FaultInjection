import fcntl
import json
import logging
import os
import shutil
from dataclasses import dataclass
from enum import IntEnum
from pathlib import Path
from subprocess import PIPE, Popen, TimeoutExpired
from sys import stderr, stdout
from threading import Lock
from typing import Dict, List, Tuple

import utils.consts as consts
import utils.fj as fj
import utils.parsers as parsers
from utils.helper import custom_run, custom_run_new
from utils.ktypes import *
import math

logger = logging.getLogger(__name__)

@dataclass
class TestCase:
    tag: str

    env: Dict

    # Runtime path
    build_lock: Lock
    build_root: Path
    test_root: Path

    fj_conf: Dict

    test_def: TestDef

    _is_build: bool = False
    _is_finished: bool = False

    @property
    def name(self) -> str:
        return f"Testcase({self.tag}):"

    @property
    def fj_conf_path(self) -> Path:
        return self.build_root / "config.json"

    # @property
    # def build_lock(self) -> Path:
    #     return self.build_root / "lock"

    @property
    def test_lock(self) -> Path:
        return self.test_root / "lock"

    def __run(self, tag:str, cmd: List[str], cwd: Path) -> Tuple[int, str, str]:  # return code, stdout, stderr
        # inject the FJ configs
        self.env["KDEBUG"] = "ON"
        self.env["FJ_CONFIG"] = self.fj_conf_path.absolute()

        logger.debug(f"exec: cmd={cmd}, cwd={cwd}")
        rc, out, err = custom_run(cmd, cwd=cwd, env=self.env)
        logger.info(f"{self.name} {tag} result: {rc}")

        return rc, out, err
    
    def __run_new(self, tag:str, cmd: List[str], cwd: Path) -> Tuple[int, str, str]:  # return code, stdout, stderr
        # inject the FJ configs
        self.env["KDEBUG"] = "ON"
        self.env["FJ_CONFIG"] = self.fj_conf_path.absolute()

        logger.debug(f"exec: cmd={cmd}, cwd={cwd}")
        rc, out, err = custom_run_new(cmd, cwd=cwd, env=self.env, output_path=self.build_root)
        logger.info(f"{self.name} {tag} result: {rc}")

        return rc, out, err

    def __lock_path(self, path: str):
        fcntl.flock(open(path, "wb+"), fcntl.LOCK_EX)
        return

    def __release_path(self, path: str):
        fcntl.flock(open(path, "wb+"), fcntl.LOCK_UN)
        return

    def build(self) -> KOutput:
        # Here we rely on syscall fcntl to control the lock
        with self.build_lock:
            fj.write_fj_conf(self.fj_conf_path, self.fj_conf)

            script_path = Path(os.getcwd()) / self.test_def.build.script

            binary_path = self.build_root / self.test_def.build.binary
            logger.debug(f"build binary_path: {binary_path}")
            binary_name = binary_path.name

            # in test_root files
            binary_in_test_root_path = self.test_root / binary_name
            binary_build_output = self.test_root / "build-output.json"

            if binary_in_test_root_path.exists() and binary_build_output.exists():
                try:
                    logger.info(f"Building test: {self.tag} Exists, bypassing")
                    with open(binary_build_output, "r", encoding="utf8") as f:
                        data = json.load(f)
                    # logger.debug(f"cached built output: {data}, {type(data)}")
                    ret = KOutput(**data)
                    self._is_build = True
                    return ret
                except Exception as e:
                    logger.error(f"Test {self.name} load cached build log failed: {e}, rebuilding")
                    self._is_build = False

            rc, out, err = self.__run(
                    tag = "build",
                    cmd = [ script_path ],
                    cwd = self.build_root,
                )
            if rc == 0:
                self._is_build = True

            ret = KOutput(retcode=rc, stdout=out, stderr=err)
            if ret.retcode != 0:
                print('INFO_HERE')
                print(self.tag)
                print(self.build_root)
                print(self.fj_conf)
                print(f"Build failed: {out}")
                print('----------------------------------')
                print(err)
                exit(-1)
            assert ret.retcode == 0, f"Build failed: {out}"

            logger.debug(f"copying built binary: {binary_path} => {self.test_root}")
            try:
                with open(binary_build_output, "w", encoding="utf8") as f:
                    json.dump(ret.model_dump(), f, ensure_ascii=False, indent=2)
                shutil.move(binary_path, self.test_root)
                shutil.move(self.fj_conf_path, self.test_root)
            except FileNotFoundError as e:
                logger.error(f"{self.name}: shutil copy error, {self}")
                # os.system(f"ls {self.build_root}/build")
                raise e

            return ret

    def build_new(self) -> KOutput:
        with self.build_lock:
            fj.write_fj_conf(self.fj_conf_path, self.fj_conf)

            script_path = Path(os.getcwd()) / self.test_def.build.script

            binary_path = self.build_root / self.test_def.build.binary
            binary_name = binary_path.name

            # in test_root files
            binary_in_test_root_path = self.test_root / binary_name
            binary_build_output = self.test_root / "build-output.json"

            if binary_in_test_root_path.exists() and binary_build_output.exists():
                try:
                    logger.info(f"Building test: {self.tag} Exists, bypassing")
                    with open(binary_build_output, "r", encoding="utf8") as f:
                        data = json.load(f)
                    # logger.debug(f"cached built output: {data}, {type(data)}")
                    ret = KOutput(**data)
                    self._is_build = True
                    return ret
                except Exception as e:
                    logger.error(f"Test {self.name} load cached build log failed: {e}, rebuilding")
                    self._is_build = False

            rc, out, err = self.__run_new(
                    tag = "build",
                    cmd = [ script_path ],
                    cwd = self.build_root,
                )
            if rc == 0:
                self._is_build = True

            ret = KOutput(retcode=rc, stdout=out, stderr=err)
            if ret.retcode != 0:
                print('INFO_HERE')
                print(self.tag)
                print(self.build_root)
                print(self.fj_conf)
                print(f"Build failed: {out}")
                print('----------------------------------')
                print(err)
                exit(-1)
            assert ret.retcode == 0, f"Build failed: {out}"

            logger.debug(f"copying built binary: {binary_path} => {self.test_root}")
            try:
                with open(binary_build_output, "w", encoding="utf8") as f:
                    json.dump(ret.model_dump(), f, ensure_ascii=False, indent=2)
                shutil.move(binary_path, self.test_root)
                shutil.move(self.fj_conf_path, self.test_root)
            except FileNotFoundError as e:
                logger.error(f"{self.name}: shutil copy error, {self}")
                # os.system(f"ls {self.build_root}/build")
                raise e

            return ret

    def test(self) -> KOutput:
        assert self._is_build, f"{self.name} is not built yet"
        logger.debug(f"{self.tag} testing...")

        test_output = self.test_root / "test-output.json"

        if test_output.exists():
            try:
                data = json.load(open(test_output, "r", encoding="utf8"))
                ret = KOutput(**data)
                return ret
            except Exception as e:
                logger.error(f"Testcase {self.tag} load cached test log failed: {e}, rebuilding")
                self._is_finished = False

        rc, out, err = self.__run(
                tag = "test",
                cmd = self.test_def.run.cmd,
                cwd = self.test_root,
            )

        try:
            ret = KOutput(retcode=rc, stdout=out, stderr=err)
        except Exception as e:
            logger.error(f"KOutput parse error: rc: {rc}, stdout: {out}, stderr: {err}, {e}")
            raise e
        with open(test_output, "w", encoding="utf8") as f:
            json.dump(ret.model_dump(), f, ensure_ascii=False, indent=2)
        return ret


def parallel_build_fn(testcase: TestCase):
    testcase.build_new()
    return

def parallel_test(uid: str, testcase: TestCase):
    koutput: KOutput = testcase.profile_testcase.test()
    profile_result = parsers.parse_run_result(koutput.retcode, koutput.stdout, koutput.stderr)
    return profile_result
