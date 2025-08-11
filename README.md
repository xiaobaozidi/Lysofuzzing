# From Alarms to Real Bugs: Multi-target Multi-step Directed Greybox Fuzzing for Static Analysis Result Verification (USENIX SECURITY 2025)

## 1. Introduction
This Git repo provides the prototype of Lyso. 
For more techniques details, check out [paper](https://www.usenix.org/system/files/conference/usenixsecurity25/sec25cycle1-prepub-1022-bao.pdf)

## 2. Run Lyso on Magma
To run Lyso on the Magma benchmark, follow the [official Magma instructions](https://hexhive.epfl.ch/magma/docs/getting-started.html) to start fuzzing.  
We provide a modified `magma_dgf` directory, which includes changes to the benchmark environment settings. Please use this version for compatibility.  

The `magma_dgf` currently supports multiple DGFs, including **Lyso**, **Titan**, **SelectFuzz**, **AFLGo**, and **FishFuzz**.  
Our goal is to integrate more DGFs into this benchmark, so feel free to contribute additional ones.



