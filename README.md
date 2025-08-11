# From Alarms to Real Bugs: Multi-target Multi-step Directed Greybox Fuzzing for Static Analysis Result Verification (USENIX SECURITY 2025)

## 1. Introduction
This Git repo provides the prototype of Lyso. 
For more techniques details, check out [paper](https://www.usenix.org/system/files/conference/usenixsecurity25/sec25cycle1-prepub-1022-bao.pdf)

## 2. Run Lyso on Magma
To run Lyso on the Magma benchmark, follow the [official Magma instructions](https://hexhive.epfl.ch/magma/docs/getting-started.html) to start fuzzing.  
We provide a modified `magma_dgf` directory, which includes changes to the benchmark environment settings. Please use this version for compatibility.  

The `magma_dgf` currently supports multiple DGFs, including **Lyso**, **Titan**, **SelectFuzz**, **AFLGo**, and **FishFuzz**.  
One challenge in evaluating DGFs is that they often use different benchmarks, making it difficult to compare their effectiveness.  
Our goal is to integrate more DGFs into a standardized benchmark, so feel free to contribute additional ones.

## 3. Lyso Structure
lyso
├── build.sh    
├── compilei\_fuzzer.sh
├── fetch.sh
├── findings.sh
├── instrument.sh
├── llvmpass
├── preinstall.sh
├── repo
├── runonce.sh
├── run.sh
├── scripts
├── src
└── targets




