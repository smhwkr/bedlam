# bedlam v2.4 (GPU, CID)

This version uses the same **two-tier load-balancing architecture** as v2.3, but attempts to increase the L1 hit rate (and therefore **reduce the latency**) when looking up bitmasks.

It introduces a compressed ID (CID), which doesn't have the power-of-two padding of the regular ID and therefore results in a smaller lookup table.

This is what the architecture looks like:

![Alt text](../assets/v2_4_architecture.svg)

This is a plot of active, producer, and consumer threads over time:

![Alt text](../assets/v2_4_chart.svg)

