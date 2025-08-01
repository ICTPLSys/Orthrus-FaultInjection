## Individual Experiment: Memcached



### Throughput (Figure 6)

**Commands:**  `just test-memcached-throughtput`

**Execution Time:** `> 20m`

**Test Results:** `results/memcached-throughtput-report.txt[.json]`

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

### Throughput vs  Latency(p95) (Figure 7)

**Commands:** `just test-memcached-latency_vs_pXX`

**Execution Time:** `> 30m`

**Test Results:** `results/memcached-latency_vs_pXX_{vanilla|orthrus|rbv}.json`

**Example:** N/A



--------------

### Validation Latency CDF (Figure 8)

**Commands:**  `just test-memcached-validation_latency_cdf`

**Execution Time:** `> 20m`

**Test Results:** `results/memcached-validation_latency-{vanilla|orthrus|rbv}.cdf`

**Example:** N/A



--------------

## Memory (in paper)

**Commands:**  `just test-memcached-memory`

**Execution Time:** `> 20m`

**Test Results:** `results/memcached-memory_status-report.txt`

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
ratio (scee vs raw):  1.0407063918915729
ratio (rbv vs raw):   1.9985104481502283
----------  results(avg)  ----------
ratio (scee vs raw):  1.0533045108399122
ratio (rbv vs raw):   1.9886216904041945

```



