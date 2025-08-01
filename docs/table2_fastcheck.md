## Fault Injection: Fast check results of Table2

The full test process for Table 2 is time-consuming. To demonstrate that the detection rates reported in Table 2 can be achieved, we use FastCheck, which generates jishi results for each entry in Table 2 with similar detection rates.

A detailed description of our fault injection methodology is available in table2_full.md. For FastCheck, run the script:

```
./table2_fastcheck.sh
```

This script outputs the fault detection rate for each application and fault tolerance mechanism.

For example, the initial output lines may appear as follows:

```

                Arithmetic     FP     Vector   Cache    
Memcached:      
RBV:            99/100(99%)   ....     ....     ....
Orthrus:          ....        ....     ....     ....

```

Notes:

(1) FastCheck produces significantly fewer SDC errors than the full test.
(2) The distribution of injected error types may not match Table 2 proportions due to the smaller sample size.
(3)Orthrus and RBV may show differing total SDC counts since additional errors are injected into Orthrus to highlight its efficiency. Detection rates remain reliable.