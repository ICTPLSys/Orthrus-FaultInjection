# Orthrus: Efficient and Timely Detection of Silent User Data Corruption

Orthrus-FaultInjection is an automated fault-injection test platform designed for research in SDC (Silent Data Corruption) detection. This repository specifically contains the **fault injection branch**, used for conducting error injection tests as described in our SOSP 2025 paper.

## Introduction

Orthrus provides a resource-adaptive computation validation mechanism to detect SDC in cloud environments efficiently. This platform automates the process of injecting faults into applications to evaluate their resilience and the effectiveness of detection mechanisms.

## Project Structure

- `AE.md`: Instructions for Artifact Evaluation of the SOSP'25 paper.
- `instructions.md`: Detailed guide on how to inject faults into custom application code.
- `contributions.md`: Guidelines for contributing to this project.
- `fj_targets/`: Target applications for fault injection.
- `fw/`: Framework scripts and tools.
- `docs/`: Additional documentation and evaluation details.

## Getting Started

### Prerequisites

- **OS**: Ubuntu 20.04 (recommended).
- **CMake**: Version >= 3.20.
- For more details, see [docs/prerequisite.md](docs/prerequisite.md).

### Installation

To set up the environment, including the modified LLVM clang compiler and the Python testing platform:

```bash
# Run as a regular user (NOT root)
./fw/scripts/table2_setup.sh
```

*Note: The LLVM installation might take some time. If it hangs during "Downloading", you may need to run the `./build.sh` manually.*

## Usage

### Running Fault Injection Tests

We provide scripts to replicate the results from Table 2 of our paper:

- **Fast Check (approx. 2 hours)**:
  ```bash
  ./fw/scripts/table2_fastcheck.sh
  ```
  See [docs/table2_fastcheck.md](docs/table2_fastcheck.md) for details.

- **Full Evaluation (approx. 40 hours)**:
  ```bash
  ./fw/scripts/table2_full.sh
  ```
  See [docs/table2_full.md](docs/table2_full.md) for details.

### Custom Fault Injection

To inject faults into your own application code, please follow the steps in [instructions.md](instructions.md).

## Contributing

Contributions are welcome! Please refer to [contributions.md](contributions.md) for more information.

## Citation

If you use Orthrus in your research, please cite our SOSP 2025 paper:

```bibtex
@inproceedings{liu2025orthrus,
  title={Orthrus: Efficient and Timely Detection of Silent User Data Corruption in the Cloud with Resource-Adaptive Computation Validation},
  author={Liu, Chenxiao and Zhu, Zhenting and Li, Quanxi and Xia, Yanwen and Qiao, Yifan and Deng, Xiangyun and Lu, Youyou and Xie, Tao and Cui, Huimin and Du, Zidong and others},
  booktitle={Proceedings of the ACM SIGOPS 31st Symposium on Operating Systems Principles},
  pages={286--304},
  year={2025}
}
```
