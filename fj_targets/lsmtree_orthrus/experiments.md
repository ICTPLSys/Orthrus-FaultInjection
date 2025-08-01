## 实验说明：



### 吞吐 Figure 6

对应 `throughput/lsmtree_throughput_{raw,scee,rbv}`

对应 `justfile`  `just test-throughtput`

执行时间：8m30s

结果输出：stdout

```
execution time: 1.06864e+08, throughput: 467883   // <----
```

Example 结果：
```
raw:  467883
scee: 436899    // 93%
rbv:  216496    // 46%
```



### 吞吐 vs  p95 延迟 Figure 7

对应 `latency/lsmtree_socket_latency_pXX_{raw,scee,rbv}`

对应 `justfile`  `just test-latency-all`

执行时间：46m35s

结果输出：

1. 中间结果：`latency_vs_pXX_{raw,scee,rbv}.log`
2. 最终数据：`latency_vs_pXX_{raw,scee,rbv}.json`
3. 画图：`scipts/tail-latency.py`

Example 结果：

![tail-latency](C:\Users\kuriko\Desktop\tail-latency.png)





### Closure 校验延迟 CDF Figure 8

对应 `latency/lsmtree_socket_latency_cdf_{raw,scee,rbv}`

对应 `justfile`  `just test-latency-cdf`

执行时间：6m49s

结果输出：

1. 数据：`validation-latency-{raw,scee,rbv}.json`
2. 画图：`scipts/validation-latency.py`

Example 结果：

![validation-cdf](C:\Users\kuriko\Desktop\validation-cdf.png)



### SDC 覆盖率 Table 2





### SDC 检测率 vs 核心数 Figure 9