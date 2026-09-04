# bedlam v2.4 (GPU, CID)

This version uses the same **two-tier load-balancing** architecture as v2.3, but uses compressed IDs (CIDs) in place of IDs. This reduces the size of the lookup table and increases the L1 cache hit rate.

This is what the architecture looks like:

![Alt text](../assets/v2_4_architecture.svg)

This is a plot of active, producer, and consumer threads over time:

![Alt text](../assets/v2_4_chart.svg)

