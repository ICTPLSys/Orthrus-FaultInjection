# Orthrus: Efficient and Timely Detection of Silent User Data Corruption in the Cloud with Resource-Adaptive Computation Validation

> This is the `fj branch` for fault injection test.

> To set up the environment:
See [docs/prerequisite.md](docs/table2_fastcheck.md)


Details in 

- [docs/table2_fastcheck.md](docs/table2_fastcheck.md)

- [docs/table2_full.md](docs/table2_full.md)

## Instructions

This repository presents Orthrus, a research artifact designed for efficient and timely detection of silent user data corruption in cloud environments. As mentioned in the paper, we validated Orthrus through two sets of performance experiments: one focusing on its overall performance and validation latency, and another on its error coverage.

1. The **complete** performance test takes approximately **4 hours**. (Figure 6-9)
2. **Error Injection Tests** (Table 2):

   2.1 A **complete** error injection test may take over **30 hours**.

   2.2 We also provide a **partial** error injection test for a fast check, which takes **2-3 hours** (Note: the results in the paper are based on the full error injection test).

## Environment and Dependencies

Refer to the document: [docs/prerequisite.md](docs/prerequisite.md).

## Performance Testing
> This section corresponds to Figures 6-9 in the paper.

The tests are divided based on the target applications: **Memcached**, **Masstree**, **Phoenix**, and **LSMTree**.

### Complete Test

In order to get the detection rate result, you need to get the fault injection result first.
Generating this result takes a long time, so we have provided a copy of the result in advance. Please download it to `result/fault_injection.tar.gz`
from https://1drv.ms/f/c/f66d2e84dd351208/Ekc-nXYZcZlPiK5CrfH7VbwBAqLTHGar_eC6JiwcbajMOg?e=e22l0M

Run the following command:

```bash
just test-all
```

The tests will run automatically, and the performance results will be saved in the `results` folder.


### Individual Tests

For the details of individual tests, please refer to the individual documents:

- [docs/experiment - memcached.md](docs/exp-memcached.md)

- [docs/experiment - masstree.md](docs/exp-masstree.md)

- [docs/experiment - phoenix.md](docs/exp-phoenix.md)

- [docs/experiment - lsmtree.md](docs/exp-lsmtree.md)


## Protection Capability Testing

Please refer to branch [fault injection (fj)](https://github.com/Orthrus-SOSP25/Orthrus-AE/tree/fj) for more information.
