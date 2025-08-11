# From Alarms to Real Bugs: Multi-target Multi-step Directed Greybox Fuzzing for Static Analysis Result Verification (USENIX SECURITY 2025)

## 1. Introduction
This Git repo provides the prototype of Lyso. 
For more techniques details, check out [paper](https://www.usenix.org/system/files/conference/usenixsecurity25/sec25cycle1-prepub-1022-bao.pdf)

## 2. Run Lyso on Magma
To run Lyso on the Magma benchmark, follow the [official Magma instructions](https://hexhive.epfl.ch/magma/docs/getting-started.html) to start fuzzing.  
We provide a modified `magma_dgf` directory, which includes changes to the benchmark environment settings. Please use this version for compatibility.  

The `magma_dgf` currently supports multiple DGFs, including **Lyso**, **Titan**, **SelectFuzz**, **AFLGo**, and **FishFuzz**.  
One challenge in evaluating DGFs is that they often use different benchmarks, making it difficult to compare their effectiveness.  
Another challenge is that setting up the fuzzing environment for DGFs can be complex and time-consuming.  

The `magma_dgf` aims to address these issues by providing a unified and ready-to-use environment.  
Our goal is to integrate more DGFs into a standardized benchmark (i.e., Magma), so feel free to contribute additional ones.

To add a new DGF to `magma_dgf`, place it under `magma_dgf/fuzzers` and follow the standard structure by providing:  
- `build.sh`  
- `fetch.sh`  
- `findings.sh`  
- `instrument.sh`  
- `preinstall.sh`  
- `runonce.sh`  
- `run.sh`  
- Any additional logic or scripts specific to the new DGF

## 3. Lyso Structure
```
lyso
├── build.sh    
├── compile_fuzzer.sh
├── fetch.sh
├── findings.sh
├── instrument.sh
├── llvmpass # Lyso's graph construction
├── preinstall.sh
├── repo # Lyso's main fuzzing loop, step tracking and instrumentation
├── runonce.sh
├── run.sh
├── scripts # Lyso's distance calculation
├── src # Fuzzer driver
└── targets # Static analysis result for magma target
```

## 4. Run Lyso on other programs

### Install Dependency

```
#!/bin/bash
set -e

apt-get update --fix-missing && \
    apt-get install -y make build-essential git wget cmake gawk libboost-all-dev gdb 


# Dowdload llvm-11

mkdir -p /llvm-11
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.1/clang+llvm-11.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz
tar -xf clang+llvm-11.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz  -C /llvm-11 --strip-components=1
rm clang+llvm-11.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz

apt-get install -y python3 python3-dev python3-pip
pip3 install --upgrade pip
pip3 install networkx pydot
pip3 install wllvm

```

### build Graph Construction

```
cd llvmpass
./compile_graphpass.sh

```

### 

