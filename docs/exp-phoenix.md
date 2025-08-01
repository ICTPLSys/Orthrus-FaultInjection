## Individual Experiment: Phoenix




### Throughput (Figure 6)

**Commands:**  `just test-phoenix-throughtput`

**Execution Time:** `2m43s`

**Test Results:** `results/phoenix-throughtput-report.txt[.json]`

**Example:**

```
vanilla running
    Time taken: 28757 ms
orthrus running
    Time taken: 28783 ms
rbv running
    Time taken: 36576 ms
```



--------------

### Validation Latency CDF (Figure 8)

**Commands:**  `just test-phoenix-validation_latency_cdf`

**Execution Time:** `19m51s`

**Test Results:** `results/phoenix-validation_latency-{vanilla|orthrus|rbv}.cdf`

**Example:** N/A



--------------

## Memory (in paper)

**Commands:**  `just test-phoenix-memory`

**Execution Time:** `4m36s`

**Test Results:** `results/phoenix-memory_status-report.txt`

**Example:** 

```
=== Memory Stats ===
Processing raw
max mem run :  43030392
Processing scee
max mem run :  44782004
Processing rbv
max mem run :  42970432
max mem run :  43026256
----------  results(peak)  ----------
ratio (scee vs raw):  1.0407063918915729  # aka, 4%
ratio (rbv vs raw):   1.9985104481502283  # aka, 100%
----------  results(avg)  ----------
ratio (scee vs raw):  1.0533045108399122
ratio (rbv vs raw):   1.9886216904041945

```

