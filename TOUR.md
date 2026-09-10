This file includes a tour of the files/directories present within this repository, since the repository is kind of a mess at the moment, I apologize.

# OpenMP Subdirectory
The OpenMP subdirectory, containing the OpenMP part of this project, contains a relatively normal C project, with the main oddity being a bash script being used for compilation (the bash script has been written with the goal of being useable with older versions of bash, such as the bash `3.2` shipped with MacOS, but this hasn't been tested due to the author lacking access to the older versions of bash in question).

Files of interest in the `openmp` subdirectory include
- `comp`: bash script to compile the openmp code, to be invoked as `./comp all`
- `bench`: bash script to benchmark the openmp code, should be called after `./comp all` has been done
- `build`: build directory where `comp` stores output binary files
- `results`: results directory where `bench` stores benchmark results
- `raylib-*`: the openmp project includes both
  - graphical: displays the boids, but doesn't compute any performance stats, and
  - benchmark: computes performance stats but doesn't display anything
  versions of the the boids program, for the graphical versions of the program raylib was used as the library to handle graphics for the program
- `src` | `include`: normal c division of a program into `.h` and `.c` files, source files present therein which are of interest for this tour include:
  - `aos.(c|h)`: include the logic for the array-of-structs version of the boids update loop
  - `soa.(c|h)`: include the logic for the struct-of-arrays version of the boids update loop
  there are no separate files to distinguish between the parallelized and the sequential versions of the `soa` and `aos` versions of the program, parallelism and sequentiality are controlled only by turning the appropriate openmp flags on or off, this causes warnings in the sequential version for unrecognized pragmas but these warnings are not a problem for the running program
  - `argparse.(c|h)`: to run the program with different parameters without having to recompile it every time we have developed a small c library to parse command line arguments
  - `config.(c|h)`: defines a configuration struct containing the aphorementioned parameters
  - `args.(c|h)`: code using the the `argparse` library to populate `config` structs 
  - `benchmark-*.(c|h)`: the benchmark results for this file are stored as `.csv` files
    - `benchmark-common.(c|h)`: common logic for timing and producing `.csv` files
    - `benchmark-aos.c`: main function benchmarking `aos` version of the program
    - `benchmark-soa.c`: main function benchmarking `soa` version of the program
  - `graphics-*.(c|h)`: code to handle graphics

# OpenCL Subdirectory
## Initial Notice (resons for which it has a `Dockerfile`)
The choice of OpenCL for the GPU program has not been without cost, and has also affected the directory structure of the OpenCL subproject, this is due to the fact the maintainers of fedora (the operating system used by the author) [decided to drop OpenCL support for my card while I was developing this project](https://fedoraproject.org/wiki/Changes/IntelCompute2025), leading to development of the OpenCL part having to continue through use of Docker.

To leverage this (unwilling) choice of technology the development container has been designed to be rather ephemeral, to make the development environment more reproducible, it runs, has some shared (`--bind`) directories with the host where the project files are written during development, and it is later deleted (`docker run -rm`).

The docker container used does **not** follow the [devcontainer](https://containers.dev/overview) spec since the author didn't need it.

## Directory Structure
Files of interest in the `opencl` subdirectory include:
- `Dockerfile`: the dockerfile used to create the image used to create the development container used to develop the opencl part
- `container`: a bash script abstracting all the convoluted `docker` cli calls away f from the user so they can just use the container, script may be used as
  - `./container build`: builds image from `Dockerfile`
  - `./container run` (or just  `./container`): runs image and drops into a bash prompt from which one may interact with running container
- `images`: since the OpenCL code implements histogram equalization, it needed test images to run on
- `results`: directory for benchmark results
- `c-workspace`: (virtually defunct) workspace directory where the final c port of the project was meant to go
- `python-workspace`: subdirectory containing the code for the `pyopencl` version of the project, files/directories of interest therein include
  - `scratchpad/`: files created during development and later discarded, versioned for potential utility, include some one-and-done test code, one-and-done benchmark code, old kernels, old utilities, and prototype code written in [alien heiroglyphs](https://mlochbaum.github.io/BQN/)
  - `pipeline.txt`: diagram of data flow for opencl code, but as a text file
  - `utils.py`: contains timing, math, and opencl utility functions used for the tests and benchmarks
  - `cl_histbench.py`: benchmark code for the opencl version of histogram equalization
  - `np_histbench.py`: benchmark code for a sequential version of histogram equalization written in numpy
  - `bench.sh`: posix shell script that runs the two files above
  - `*.cl`, kernel code used by `cl_histbench.py`, includes
    - `color_space_conversions.cl`: code to convert an image from rgb(a) color space to and from hsl(a) color space
    - `private_naive_histograms.cl`: histogram kernels to compute histograms of whole image or just a channel of the image
    - `scans.cl`: kernels to compute plus scan of histogram (required by equalization step) 
    - `equalization.cl`: kernel to take an image and a (channel's) histogram and output the equalized the image

# Tools Subdirectory
- `utils.bash`: coloring and logging utilities for shell scripts (used by `opencl/container`)
- `shsl`: small lisp interpreter initially developed to replace the bash scripts in this repository but later not adopted due to timing constraints

# Report Subdirectory
> hic sunt leones
