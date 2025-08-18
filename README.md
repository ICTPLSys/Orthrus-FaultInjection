# Orthrus: Efficient and Timely Detection of Silent User Data Corruption in the Cloud with Resource-Adaptive Computation Validation

> This is the `fault injection branch` for fault injection test. !!! This branch's README is not same as master branch!!!

This branch is for **Error Injection Tests** (Table 2):

1. A **complete** error injection test may take about **40 hours**. (table2_full.sh) 
2. We also provide a **partial** error injection test for a fast check (table2_full.sh), which takes **2 hours** (Note: the results in the paper are based on the full error injection test).

> To set up the environment:
See [docs/prerequisite.md](docs/prerequisite.md)
```shell
# Run in User mode, not in ROOT
./fw/scripts/table2_setup.sh
```

> Details for evaluation in:

- [docs/table2_fastcheck.md](docs/table2_fastcheck.md)

```shell
# Run in User mode, not in ROOT
./fw/scripts/table2_fastcheck.sh
```

- [docs/table2_full.md](docs/table2_full.md)

```shell
# Run in User mode, not in ROOT
./fw/scripts/table2_full.sh
```

This repository presents Orthrus, a research artifact designed for efficient and timely detection of silent user data corruption in cloud environments. As mentioned in the paper, we validated Orthrus through two sets of performance experiments: one focusing on its overall performance and validation latency, and another on its error coverage.