# From Alarms to Real Bugs: Multi-target Multi-step Directed Greybox Fuzzing for Static Analysis Result Verification (USENIX SECURITY 2025)

## 1. Introduction

This Git repo provides the prototype of Lyso. For more technical details, check out the [paper](https://www.usenix.org/system/files/conference/usenixsecurity25/sec25cycle1-prepub-1022-bao.pdf).

## 2. Run Lyso on Magma

To run Lyso on the Magma benchmark, follow the [official Magma instructions](https://hexhive.epfl.ch/magma/docs/getting-started.html) to start fuzzing.

We provide a modified `magma_dgf` directory, which includes changes to the benchmark environment settings. Please use this version for compatibility.

The `magma_dgf` currently supports multiple DGFs, including:
- **Lyso**
- **Titan** 
- **SelectFuzz**
- **AFLGo**
- **FishFuzz**

### Challenges in DGF Evaluation

One challenge in evaluating DGFs is that they often use different benchmarks, making it difficult to compare their effectiveness. Another challenge is that setting up the fuzzing environment for DGFs can be complex and time-consuming.

The `magma_dgf` aims to address these issues by providing a unified and ready-to-use environment. Our goal is to integrate more DGFs into a standardized benchmark (i.e., Magma), so feel free to contribute additional ones.

### Adding New DGFs

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
├── llvmpass         # Lyso's graph construction
├── preinstall.sh
├── repo             # Lyso's main fuzzing loop, step tracking and instrumentation
├── runonce.sh
├── run.sh
├── scripts          # Lyso's distance calculation
├── src              # Fuzzer driver
└── targets          # Static analysis result for magma target
```

## 4. Run Lyso on Other Programs

### 4.1 Install Dependencies

```bash
#!/bin/bash
set -e

apt-get update --fix-missing && \
    apt-get install -y make build-essential git wget cmake gawk libboost-all-dev gdb 

# Download llvm-11
mkdir -p /llvm-11
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.1/clang+llvm-11.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz
tar -xf clang+llvm-11.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz  -C /llvm-11 --strip-components=1
rm clang+llvm-11.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz

apt-get install -y python3 python3-dev python3-pip
pip3 install --upgrade pip
pip3 install networkx pydot
pip3 install wllvm
```

### 4.2 Build Graph Construction

```bash
cd llvmpass
./compile_graphpass.sh
```

### 4.3 Build Lyso Fuzzer

```bash
./compile_fuzzer.sh
```

### 4.4 Generate Target Traces

Lyso relies on static analysis results (i.e., source–sink traces) to achieve step-by-step guidance and avoid unnecessary exploration. Therefore, if you want to run Lyso on another program, first perform static analysis (e.g., using CodeQL, Infer, or another static analysis tool) to obtain the required source–sink traces.

### 4.5 Generate Bitcode File

Use LLVM to generate bitcode file for the target program:

```bash
export CC="wllvm"
export CXX="wllvm++"
export LLVM_COMPILER=clang
./build.sh ## Usually the build script for the project 
```

### 4.6 Insert ID

The `afl-llvm-pass.so` is used to insert a unique ID into each basic block, enabling step tracking at runtime. These inserted IDs correspond to the IDs used in distance calculation, allowing us to precisely measure the target distance during execution.

```bash
lyso/repo/afl-llvm-pass.so -test -mode=is_insert $bc_path -o $OUT/$p-id.bc
```

### 4.7 Distance Calculation

```bash
opt -load $GRAPHPASS -GraphPass -targets=target.txt -outdir=$folder $bc_path
$SCRIPTS/distance_calculation_cg.py $SCRIPTS $folder
$SCRIPTS/distance_calculation_cfg.py $SCRIPTS $folder
```

Where:
- `$GRAPHPASS` is the graph construction pass built in Section 4.2
- `target.txt` file is the target trace file generated in Section 4.4
- `$bc_path` is the bitcode file generated in Section 4.5

Lyso's distance calculation operates at the basic block level. We first compute distances at the function level (i.e., call graph), and then at the basic block level (i.e., control flow graph). The distance calculation computes every (source, target) pair in the program and stores the results.

At runtime, during step tracking, Lyso fetches the stored distances and calculates the shortest distance from the current execution to the next uncovered step.

### 4.8 Instrumentation

```bash
dist_file=$folder/distance.cfg.txt
trace_file=$folder/IntraBB.txt
bc_path=$(find $OUT -name "$p-id.bc")
lyso/repo/afl-clang-fast++ $CXXFLAGS -std=c++11 \
  -outdir=$folder \
  -trace=$trace_file \
  -distance=$dist_file \
  -I. $bc_path -o $OUT/$p.fuzz
```

Parameters:
- `dist_file` — Distance calculation output from Section 4.7
- `trace_file` — Target trace file (converted to unique IDs) from Section 4.7
- `bc_path` — Path to the ID-inserted bitcode generated in Section 4.6

### 4.9 Run Lyso

```bash
lyso/repo/afl-fuzz -d -g out -c "$SHARED" -m 100M -i corpus -- p.fuzz $ARGS 2>&1
```

Parameters:
- `-g out` — Temporary directory for runtime results


## Paper Artifact

Our artifact is also aviable on [Zenodo] (https://zenodo.org/records/14714504). In contains the TTR (Time-to-Reach) and TTE (Time-to-Exposure) data for Table 2 in our paper.

## Contributing

We welcome contributions to improve Lyso and integrate additional DGFs into the `magma_dgf` environment. Please feel free to submit pull requests or open issues for any bugs or feature requests.

## Citation

If you use Lyso or our benchmark `magma_dgf` in your research, please cite our paper:

```bibtex
@article{baoalarms,
  title={From Alarms to Real Bugs: Multi-target Multi-step Directed Greybox Fuzzing for Static Analysis Result Verification},
  author={Bao, Andrew and Zhao, Wenjia and Wang, Yanhao and Cheng, Yueqiang and McCamant, Stephen and Yew, Pen-Chung}
}
```
