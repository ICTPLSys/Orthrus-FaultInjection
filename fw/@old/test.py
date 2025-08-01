import json
import argparse
import logging
from multiprocessing import Pool, Queue
from subprocess import Popen, PIPE
from pprint import pprint
from pathlib import Path
from typing import Optional
import utils
from utils import *
from utils.ktypes import *

'''
Procedure:
1. Which function to inspect.
2. Build profile and injection configs.
3. Run
'''

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)


# def stage_3_inspection(cfg: Config):
#   #
#   for pc in range(100):
#     conf = build_fj_conf("injection", target_f, pc)
#     write_conf(conf)
#     build_test()
#     run_test()
#     parse_result()
#
#   return


def main(args):
    print("Hello World")

    cfg = Config(
        test_exec = Path(args.test_exec),
        scee_dir = Path(args.scee_dir),
        temp_dir = Path(args.temp_dir),
        llvm_dir = Path(args.llvm_dir),
        verbose = args.verbose,
        threads = args.threads,
    )

    run_test(cfg)

    return

if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    # where we build scee tests
    # You can use a symbolic link here
    parser.add_argument("--test-exec", type=str)

    parser.add_argument("--scee-dir", type=str)

    parser.add_argument("--llvm_dir", type=str, default="~/.local/opt/llvm16")

    # FJ config place
    parser.add_argument("--temp-dir", type=str, default="temp")

    parser.add_argument("--template", type=str, required=True)

    parser.add_argument("-t", "--threads", type=int, default=1)

    parser.add_argument("-v", "--verbose", action="store_true", default=False)

    args = parser.parse_args()

    logger.debug(args)

    with open(args.template, "r") as f:
        t = json.load(f)

    utils.template = dict(t)

    logger.debug(utils.template)

    main(args)
