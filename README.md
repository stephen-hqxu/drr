# Discrete Region Representation

[![Paper](https://img.shields.io/badge/Paper-cornflowerblue?style=for-the-badge)](https://www.computer.org/csdl/journal/tg)
[![Notebook](https://img.shields.io/badge/Notebook-cornflowerblue?style=for-the-badge)](https://github.com/stephen-hqxu/drr/blob/master/Documentation/drr.ipynb)

*Discrete Region Representation* (DRR) is a generic implicit and explicit representation to model a finite set of discrete features. Features are organised in regions defined by an integrable regionfield function, and each region is featured by a characteristic function, which can then be combined into a single continuous function selected by regionfield.

DRR is useful in procedural modelling of diverse landscape, a.k.a. multi-biome landscape. In particular, biome distribution is defined by a regionfield, and each biome has its own terrain data (typically in the form of a heightmap). The problem arises when we need to "sew" multiple patches of terrain data into one large piece without leaving noticeable seams joining those patches, while doing so efficiently to allow real-time rendering. This is smartly achieved with a weighting filter, and powered by two commonly used filter optimisations, being separation and accumulation, to cut down the asymptotic time complexity to constant with respect to filter radius.

This repository contains the implementation of the magic filter. We keep all of our experiment code in this repository, so results presented in our paper can be reproduced accordingly. We also maintain a jupyter notebook with in-depth explanations and visualisations.

## Build Instruction

### Requirement

- A **C++23** capable compiler
- CMake **v3.20 or higher**

This project makes use of [nanobench](https://github.com/martinus/nanobench/tree/v4.3.11) for measuring execution time in our experiment. We have included all dependencies in the repository and no additional setup is required.

### Compilation

Start by cloning the repository to your computer, and generate the build tree using CMake:

```sh
git clone -b master --depth 1 https://github.com/stephen-hqxu/drr

cd drr
cmake -S . -B build
```

Then, you can build the application via command line (recommended). On Windows, you may also open up the generated Visual Studio solution file located under the build directory and build the solution.

```sh
cd build
cmake --build . --config Release --target ALL_BUILD -j 3
```

After successful build, the binary is at `<project root>/build/output/bin/Release/Launch`, and can be executed without command line argument. The program will run a benchmark. Benchmark results are stored in the same place as the binary.