## Fault Injection: Full results of Table2

### This file is for instructions of full check of Table 2.

You can run the script table2_full.sh or execute its component shell scripts individually. Since the full check requires a long runtime and may crash during execution, running the scripts separately is strongly recommended.

For each application (e.g., lsmtree) and detection method (e.g., Orthrus), thousands of faults are injected to build a fault library. The injected binaries are then executed.

The output format matches that of FastCheck; however, the total Silent Data Corruption (SDC) counts may be equal to or exceed those reported in Table 2. Detection rates higher than those in the table are expected.

Notes:

- The script maintains high CPU utilization throughout its execution.
- Orthrus and RBV may yield different total SDC counts since Orthrus undergoes additional error injections to demonstrate its effectiveness. Detection rates remain reliable. Also, The SDC errors' count can be more than numbers in Table2, it's normal.