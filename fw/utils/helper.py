from functools import wraps
import logging
from subprocess import PIPE, Popen, TimeoutExpired, check_output
from sys import stderr, stdout
from typing import Tuple

from utils.ktypes import FJType
from . import consts
import select
import time
import os
import io
from tqdm import tqdm
import signal
from utils import consts
import multiprocessing
from multiprocessing import Process

logger = logging.getLogger(__name__)

def custom_run_new(cmd, cwd, env, output_path, **kwargs) -> Tuple[int, str, str]:
    # logger.debug(f"Custom run new: {cmd}, {cwd}, {env}, output_path: {output_path}, {kwargs}")
    # start_time = time.time()

    stdout_file_path = os.path.join(output_path, "stdout")
    stderr_file_path = os.path.join(output_path, "stderr")
    with open(stdout_file_path, 'w', encoding='utf-8') as stdout_file, \
         open(stderr_file_path, 'w', encoding='utf-8') as stderr_file:
        p = Popen(cmd, cwd=cwd, env=env, stdout=stdout_file, stderr=stderr_file, **kwargs)
        rc = 0
        try:
            rc = p.wait()
        except TimeoutExpired:
            rc = -4
            p.kill()
        except Exception as e:
            rc = p.returncode
            logger.debug(f"{cmd} execute failed: {e}", exc_info=True)
        finally:
            p.kill()
            
        stdout_lines = int(check_output(["wc", "-l", stdout_file.name]).split()[0])
        logger.debug(f"stdout_lines: {stdout_lines}")

        # out_lines = []
        # with open(stdout_file.name, 'r', encoding='utf-8', errors='replace') as f:
        #     with tqdm(total=stdout_lines, desc="Reading stdout", unit="lines") as pbar:
        #         for line in f:
        #             out_lines.append(line)
        #             pbar.update(1)
        # out = ''.join(out_lines)
        with open(stdout_file.name, 'r', encoding='utf-8', errors='replace') as f:
            out = f.read()
        with open(stderr_file.name, 'r', encoding='utf-8', errors='replace') as f:
            err = f.read()
    return rc, out, err


def custom_run(cmd, cwd, env, **kwargs) -> Tuple[int, str, str]:
    # logger.debug(f"Custom run: {cmd}, {cwd}, {env}, {kwargs}")
    result_dict = multiprocessing.Manager().dict()
    logger.debug(f"Custom run: {cmd}, {cwd}")
    
    # add a process to handle the timeout
    timeout_str = f"{consts.MAX_TIMEOUT}m"
    cmd = ['timeout', timeout_str] + [str(cmd[0])] + cmd[1:]
    
    logger.debug(f"cmd is {cmd}")
    p = Popen(cmd, cwd=cwd, env=env, stdout=PIPE, stderr=PIPE, **kwargs)

    rc = 0
    out = err = ""
    try:
        stdout_output = []
        stderr_output = []
        while True:
            reads = [p.stdout.fileno(), p.stderr.fileno()]
            ret = select.select(reads, [], [])

            for fd in ret[0]:
                if fd == p.stdout.fileno():
                    try:
                        line = p.stdout.readline().decode('utf-8')
                    except:
                        line = "[invalid bytes]"
                    if line:
                        # if consts.FLAG_debug:
                        #     print(f"{line}", end='')      # Print real-time stdout
                        stdout_output.append(line)    # Store in stdout list
                elif fd == p.stderr.fileno():
                    try:
                        line = p.stderr.readline().decode('utf-8')
                    except:
                        line = "[invalid bytes]"
                    if line:
                        # if consts.FLAG_debug:
                        #     print(f"{line}", end='')  # Print real-time stderr
                        stderr_output.append(line)       # Store in stderr list

            if p.poll() is not None:
                break

        p.stdout.close()
        p.stderr.close()

        out = ''.join(stdout_output)
        err = ''.join(stderr_output)
        rc = p.returncode
    except TimeoutExpired as e:
        rc = -4 # reset retcode to -4 to indicate timeout
    except Exception as e:
        rc = p.returncode
        logger.debug(f"{cmd} execute failed: {e}", exc_info=True)
    finally:
        p.kill()

    return rc, out, err


def make_uid(fname: str, pc: int):
    return f"fn({fname})|pc({pc})"


def log_func(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        logger.info(f"Running {func.__name__}")
        return func(*args, **kwargs)
    return wrapper

def timing(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        start_time = time.perf_counter()
        result = func(*args, **kwargs)
        end_time = time.perf_counter()
        elapsed_time = end_time - start_time
        logger.info(f"Function '{func.__name__}' executed in {elapsed_time:.6f} seconds")
        return result
    return wrapper
