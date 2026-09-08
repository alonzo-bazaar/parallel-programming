#!/usr/bin/env sh
d="$(dirname "$0")"
mkdir -p "${d}/../numpy.csv"
mkdir -p "${d}/../opencl.csv"
"${d}/np_histbench.py"
"${d}/cl_histbench.py"
