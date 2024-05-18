# Discrete Region Representation

[![Video](https://img.shields.io/badge/Video-cornflowerblue?style=for-the-badge)](https://youtu.be/konnTNq0IjA)
[![Notebook](https://img.shields.io/badge/Notebook-cornflowerblue?style=for-the-badge)](https://github.com/stephen-hqxu/drr/blob/master/Documentation/drr.ipynb)

*Discrete Region Representation* (DRR) is a generic implicit and explicit representation for modelling a finite set of discrete features. Features are organised into regions defined by an integrable regionfield function, and each region is characterised by a characteristic function, which can then be combined into a single continuous function selected by regionfield.

DRR is useful for procedural modelling of diverse landscapes, a.k.a. multi-biome landscapes. In particular, the distribution of biomes is defined by a regionfield function, and each biome has its own terrain data (typically in the form of a heightmap). The problem arises when we need to "stitch" multiple patches of terrain data into one large piece without leaving noticeable seams between the patches, while doing so efficiently to allow real-time rendering. This is cleverly achieved with a weighting filter, and supported by two commonly used filter optimisations, separation and accumulation, to reduce the asymptotic time complexity to constant with respect to the filter radius.

This repo contains the implementation of the filter. We keep all our experimental code with the same settings in this repo, so that the results presented in our paper can be reproduced accordingly. We also maintain a Jupyter notebook with explanations and visualisations. The concepts and rendered results are adapted from the original terrain engine in the [SuperTerrain+](https://github.com/stephen-hqxu/superterrainplus) project. Please note that the terrain engine from that repo is an early adoption and we have reworked the original engine in significant ways including restructuring the engine architecture using different graphics and compute backends. However, the bottom-level logics such as the algorithms used for terrain generation and rendering are unchanged, so we guarantee that the demos look similar to the renderings produced by the original engine.

## Build Instruction

### Requirement

- A **C++23** capable compiler. As of May 2024, only LLVM Clang 18 with Microsoft STL on Windows meet this requirement.
- CMake **v3.20 or higher**

This project makes use of [nanobench](https://github.com/martinus/nanobench) for measuring execution time in our experiment. We have included all dependencies in the repo and no additional setup is required.

### Compilation

Start by cloning the repo to your computer:

```sh
git clone -b master --depth 1 https://github.com/stephen-hqxu/drr
cd drr
```

Then, you can generate the build tree and build the project following the standard CMake build process. After successful build, the binary is at `<project root>/build/output/bin/Release`, and can be executed without command line argument. The program will run a benchmark. Benchmark results are stored in the same place as the binary.