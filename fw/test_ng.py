import argparse
import asyncio
import json
import logging
import os
import random
from datetime import datetime
from subprocess import Popen
import time
import coloredlogs

import utils
from utils import consts, parsers

FORMAT = "[%(filename)s:%(lineno)s - %(funcName)s() ] %(message)s"
coloredlogs.install(
    level=os.environ.get('LOG_LEVEL', 'INFO'),
    format=FORMAT,
)
logging.basicConfig(
    level=os.environ.get('LOG_LEVEL', 'INFO'),
    format=FORMAT
)

logger = logging.getLogger(__name__)

logger.error("ERROR Mark")
logger.debug("DEBUG Mark")
logger.warning("WARNING Mark")
logger.info("INFO Mark")

def main():
    # use the date and time as the tag
    TAG = f"scee_test_{datetime.now().strftime('%Y_%m_%d_%H_%M_%S')}"

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--test-type", type=str, required=True,
        choices=["test", "hashmap", "btree", "lsmtree", "map", "float_test", "float_test2", "simd1_test", "simd3_test", "simd4_test", "cache1", "cache2", "wordcount", "masstree", "lsmtree_rbv", "masstree_rbv", "hashmap_rbv", "wordcount_rbv"],
    )
    parser.add_argument("--tag", type=str, default=TAG)
    parser.add_argument("--scee-dir", type=str, default="../fi_hash")
    parser.add_argument("--llvm-dir", type=str, default="/home/whzzt/.local/opt/llvm16")
    parser.add_argument("--temp-dir", type=str, default="/tmp/temp")
    parser.add_argument("--category", type=str, default="computational", help="computational, consistency")
    parser.add_argument("--t-build", type=int, default=1, help="parallel when building")
    parser.add_argument("--t-test", type=int, default=32, help="parallel when testing")
    parser.add_argument("--debug", action="store_true", default=False)
    parser.add_argument("--fj-functions", type=str, default="", help="onlyapp, onlyscee, both")
    parser.add_argument("--ft-type", type=str, default="orthrus", help="orthrus, rbv")

    parser.add_argument("--output", type=str, default="output.json")
    parser.add_argument("--test_mode", type=str, default="full", help="lite, full")

    args = parser.parse_args()

    logger.info(args)

    consts.FLAG_debug = args.debug

    config = utils.Config(
        tag=args.tag,
        output=args.output,
        test_type=args.test_type,
        category=args.category,
        llvm_dir=args.llvm_dir,
        temp_dir=args.temp_dir,
        scee_dir=args.scee_dir,
        is_debug=args.debug,
        parallel_test=args.t_test,
        parallel_build=args.t_build,
        fj_functions=args.fj_functions,
        app_name=args.test_type,
        ft_type=args.ft_type,
        test_mode=args.test_mode,
    )

    parsers.init_fj_functions(args.fj_functions)

    start_time = time.time()
    # config.run()
    config.run_parallel(separate_profile=True)
    end_time = time.time()
    logger.info(f"Time taken for run: {end_time - start_time} seconds")


if __name__ == "__main__":
    main()
