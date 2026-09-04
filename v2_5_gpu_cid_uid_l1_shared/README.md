# bedlam v2.5 (GPU, CID/UID, L1 & shared)

This version uses the same **two-tier load-balancing architecture** as v2.4, but attempts to completely **eliminate the L2 cache latency** incurred when looking up bitmasks.

It introduces a compressed ID (CID) which does away with the power-of-two padding of the regular ID.
It replaces the one-step lookup with a two-step lookup, which converts the CID to a unique ID (UID) before performing the final lookup using the UID.

This is what the architecture looks like:

![Alt text](../assets/v2_5_architecture.svg)

This is a plot of active, producer, and consumer threads over time:

![Alt text](../assets/v2_5_chart.svg)

This version trades an L2 cache read for an L1 cache read followed by a shared memory read (with bank conflicts likely).
It also sacrifices occupancy and has a slightly higher instruction count.
The overall result is a very marginal performance improvement.
