# magma_dgf: A Ground-Truth Directed Fuzzing Benchmark
We provide a modified magma_dgf based on magma, which includes changes to the benchmark environment settings.

## Challenges in DGF Evaluation

One challenge in evaluating DGFs is that they often use different benchmarks, making it difficult to compare their effectiveness. Another challenge is that setting up the fuzzing environment for DGFs can be complex and time-consuming.

The `magma_dgf` aims to address these issues by providing a unified and ready-to-use environment. Our goal is to integrate more DGFs into a standardized benchmark (i.e., Magma), so feel free to contribute additional ones.

The `magma_dgf` currently supports multiple DGFs, including:
- **Lyso**
- **Titan**
- **SelectFuzz**
- **AFLGo**
- **FishFuzz**

