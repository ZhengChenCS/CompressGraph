# PRGC vs Laconic — SPEC experiment 2

This comparison branch starts at CompressGraph `d2e7bb97f20ccf4880b8d5f58f10bd8127837d1c`, before the ICDE ablation commits. The compression algorithms are retained; comparison adapters, reproducibility records, and author-code patches are additive.

Pinned Laconic: `xuqianmamba/Laconic@b38f8bfbfeb54ca2e787fe9a4971a2aa00efa1a7`.

Scope: eight-graph CPU/GPU/serial/Laconic comparison; independently varied worker threads, outer batches, and enforceable host-memory budgets; separate rule and complete-representation timing; real representation bytes and full decoding; representative matched-size measurements before any equal-quality speed claim.

Acceptance comes before benchmark results. Missing stages, failed decoding, unenforceable memory limits, and contaminated timings are recorded explicitly. The protocol and scripts in this directory contain the executable configuration and evidence.
