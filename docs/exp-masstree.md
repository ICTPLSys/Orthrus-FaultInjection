## Individual Experiment: Masstree



### Throughput (Figure 6)

**Commands:**  `just test-masstree-throughtput`

**Execution Time:** `5m41s`

**Test Results:** `results/masstree-throughtput-report.txt.json`

**Example:**

```
{
    "vanilla": {
        "throughput": "596000"
    },
    "orthrus": {
        "throughput": "559000"
    },
    "rbv": {
        "throughput": "193000"
    }
}
```



--------------

### Validation Latency CDF (Figure 8)

**Commands:**  `just test-masstree-validation_latency_cdf`

**Execution Time:** `5m51s`

**Test Results:** `results/masstree-validation_latency-{vanilla|orthrus|rbv}.cdf`

**Example:** `fig.8`



--------------

## Memory (in paper)

**Commands:**  `just test-masstree-memory`

**Execution Time:** `5m10s`

**Test Results:** `results/masstree-memory_status-report.txt`

**Example:** 

```
=== Memory Stats ===
Processing raw
max mem run :  26822820
Processing scee
max mem run :  28625716
Processing rbv
max mem run :  42271228
----------  results(peak)  ----------
ratio (Orthrus vs Vanilla):  1.0672150057301955
ratio (RBV vs Vanilla):      1.5759427233974652
----------  results(avg)  ----------
ratio (Orthrus vs Vanilla):  1.0683811019008804
ratio (RBV vs Vanilla):      1.5767581843097018

```



