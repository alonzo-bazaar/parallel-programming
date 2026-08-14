# Boids
An implementation of boids

## A Note on Realism
The first versions of the code started from a somewhat brutal reimplementatoin in c of [ben eater's boids program](https://github.com/beneater/boids)
and the current versions of the program do keep the same update logic as what is found in ben eater's code, which is a somewhat simplified take on the more traditional boids update logic, where instead of modifying the boid's speed and angle separately to keep its motion realistic, we act directly on the boid's speed vector with a bunch of forces going hither and thither.

Reading the orignal paper in which boids were presented one might see that the questionable realism of this approach is the ratio essendi itself of the boids algorithm, one may therefore conclude that this repository defeats its own purpose.

This is, at a first glance, true, but it is to be noted that the ratio essendi of this repo lies less in the realism of the behaviour implemented, and more in the parallel programming aspects of implementing it, which don't change much between this version and the one presented in the paper.

I apologize for the confusion this may have brought

# Repo Structure
Below is a list of files and directories of interest in the repository and their purpose
- `./comp` bash script to build the code
- `./bench` bash script to benchmark the code (please run `./comp` first)
- `./download-raylib` bash script to download raylib in case the raylib directories get deleted for some reason
- `./build/` build directory where `./comp` puts the output binaries (be it executables or libraries, although we currently don't generate any libraries)
- `./results/` directory where `./bench` puts the results of the various benchmark runs
- `./src/` and `./include/` are `./src/` and `./include/`

The code in this repo uses the venerable raylib for its graphics, there are also two library directories vendored within this repo, raylib for amd64-linux (which the author uses), and raylib for macos, the github releases page for raylib only has one macos library so the author lacks any idea on wether that only works on intel macbooks, `M[0-9]+` macbooks, or for both kinds of macbook, the author furthermore lacks access to an apple machine to test this.  
That is to say, it probably runs on mac, but I have no idea.  
It also doesn't run on wayland unless xwayland is enabled.  

`./src/` and `./include/` contain the following
- `./include/aos.h` and `./src/aos.c` contain the common AoS logic used for
  - `./src/graphical-aos.c`
  - `./src/benchmark-aos.c`
- `./include/soa.h` and `./src/soa.c` likewise contain the common Soa logic used for
  - `./src/graphical-soa.c`
  - `./src/benchmark-soa.c`
- `./include/graphics-common.h` and `./src/graphics-common.c` contain common graphics includes and logic shared between
  - `./src/graphical-soa.c`
  - `./src/graphical-aos.c`
- `./include/benchmark-common.h` and `./src/benchmark-common.c` likewise contain common benchmark(mainly timing and writing the result to a csv) includes and logic shared between
  - `./src/benchmark-aos.c`
  - `./src/benchmark-soa.c`
- `./include/argparse.h` and `./src/argparse.c` contain a small library for parsing `argv` that the author has written for this project, various quantities of interest for how the boids are run are passed to the main function through `argv`, as I didn't want to recompile this thing everytime I wanted to change a parameter.
- `./include/config.h` and `./src/config.c` contain a `config` struct that is passed to the boid simulation and contains various parameters for the simulation, they also contain some extra functions to set its values to default values since C doesn't let you do that for structs.
- `./include/args.h` and `./src/args.c` contain the logic to read `argv` into that config struct using the `argparse` library.

The four files
- `./src/graphical-aos.c`
- `./src/benchmark-aos.c`
- `./src/graphical-soa.c`
- `./src/benchmark-soa.c`
are the only main functions in the repo.  
There are no separate files for sequential or parllel SoA/AoS, the different behaviours are instead obtained by adding or not adding the `-fomp` flag at compile time (see the `./comp` script for more details), the code has been written in such a way as for its semantics not to change depeindg on wether `omp` is enabled or not (I only used `#pragma omp parallel` for blocks made entirely of `#pragma omp for` loops and no two iterations of a parallel loop alter the same data)

# Building
For no real reason other than the fact the author could, the code present in this repo is compiled with a bash script.  
The author would like to note that the script has been tested with bash version `5.2.32(1)-release`, since that's what I have on my machine, the script has been written in a purposefully ~~aincient~~ backwards compatible style in the hope the script may also run on a macbook, which generally ship with more paleolithic bash version such as `3.2` since they use `zsh` by default for everything.

To compile only the benchmark targets go in the repo's root directory and run
```bash
./comp benchmark
```

To compile only the graphical targets run
```bash
./comp graphics
```

To compile both benchmark and graphical targets run
```bash
./comp all
```

The resulting binaries will be found in the `./build/` subdirectory of the root directory of this repo.
There is no build cache.
