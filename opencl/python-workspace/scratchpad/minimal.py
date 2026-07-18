#!/usr/bin/env python
import numpy as np
import pyopencl as cl

import os
# not required, just makes compilation errors nicer
os.environ['PYOPENCL_COMPILER_OUTPUT']='1'

# I want to compute a histogram of an array
# I'm making it in numpy first as a baseline (and test target)
# some synthetic data
data_np=np.concatenate([np.arange(250, dtype=np.uint8),
                        np.arange(250, dtype=np.uint8),
                        np.arange(250, dtype=np.uint8)])
data_np = np.fromfile('kjv.txt', dtype=np.uint8)
baseline_hist, _ = np.histogram(data_np, np.arange(257))

# now for making it in opencl
# some shit we gotta set up to make anything opencl-y work
ctx = cl.create_some_context(interactive=False)
queue = cl.CommandQueue(ctx)

# construct kernel we're gonna run the data
# a kernel is a function marked __kernel in an opencl program
# an opencl program is this fucking thing you see below
program_source="""
__kernel void hist(const __global uchar* databuf,
                   __global uint* histbuf,
                   const uint data_size,
                   const uint hist_size) {
    const uint i = get_global_id(0);
    if(i<data_size) {
        const uint val_at_i = (uint)databuf[i];
        atomic_add(&histbuf[val_at_i], 1);
    }
}
""".strip()
program = cl.Program(ctx, program_source).build()
# pyopencl is a rather... pythony library, so all kernels in the opencl program
# get translated into fields of the pyopencl program object
# so if the program contains a function called hist, compiling it to a program
# creates a program with a field called hist
hist_kernel = program.hist

# in absolute c numerics style, the kernel function is a void that takes an input
# array pointer, an output array pointer, computes shit from the input array,
# then writes it to the output array
# so let's create an output array
hist_np=np.zeros((256,), dtype=np.uint32)

# but, since this is gonna run on the gpu, we also need input and output arrays
# on the gpu, so create a version of hist and data that will go on the gpu
data_dev = cl.Buffer(ctx, 0, data_np.nbytes)
hist_dev = cl.Buffer(ctx, 0, hist_np.nbytes)
# then copy our input data array over to the gpu array
cl.enqueue_copy(queue, data_dev, data_np)
cl.enqueue_copy(queue, hist_dev, hist_np)
queue.finish()

# we may now call the kernel
hist_kernel(queue,         # queue we'll put this kernel call on
            data_np.shape, # global work size
            None,          # local work size (here omitted)

            # remaining args are what we're gonna pass to the kernel, in the
            # order in which they appear in the kernel function's signature
            data_dev, # <- databuf (uchar* on device so array of np.uint8 on host)
            hist_dev, # <- histbuf (uint* on device so array of np.uint32 on host)
            np.uint32(data_np.shape[0]), # <- data size (uint on device so np.uint32)
            np.uint32(hist_np.shape[0])) # <- hist_size (uint on device so np.uint32)
queue.finish()

# now the array in the second parameter to the kernel call
# (which from our call to hist_kernel should be hist_dev)
# should contain the histogram of data, so let's copy it over to the cpu side
cl.enqueue_copy(queue, hist_np, hist_dev)
queue.finish()

# check that the array we wrote in the gpu (ie: hist_dev)
# then copied over to the cpu (ie: hist_np)
# contains the actual histogram of the data we started with
assert np.all(baseline_hist == hist_np), "this is where it fucks up"
