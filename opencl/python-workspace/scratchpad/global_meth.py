#!/usr/bin/env python3

# readapted from https://documen.tician.de/pyopencl/
# I just wanna test a kernel, this is waaaay more than enough
# thank you python

# histogram example
import numpy as np
import pyopencl as cl

from utils import TimedBlock, error_on_diff, flush
import os, sys

# this setting is not required for the code to function
# it's just here to make the opencl compiler give more verbose and detailed
# compiler output as to what went (most likely) wrong in compiling the kernel
# of great use for development purposes
os.environ['PYOPENCL_COMPILER_OUTPUT']='1'
# to pick the intel gpu and not the oclgrind backend
# set to 0 if you want the oclgrind backend
os.environ['PYOPENCL_CTX']='1'

# opencl setup
ctx = cl.create_some_context()
queue = cl.CommandQueue(ctx)

hist_np=np.zeros((256,), dtype=np.uint32)
data_np = np.concatenate([np.fromfile('kjv.txt', dtype=np.uint8),
                          np.fromfile('kjv.txt', dtype=np.uint8),
                          np.fromfile('kjv.txt', dtype=np.uint8),
                          np.fromfile('kjv.txt', dtype=np.uint8),
                          np.fromfile('kjv.txt', dtype=np.uint8)])
# data_np = np.concatenate([np.arange(250, dtype=np.uint8)[::-1]])
mf = cl.mem_flags

data_dev = cl.Buffer(ctx, 0, data_np.nbytes)
hist_dev = cl.Buffer(ctx, 0, hist_np.nbytes)
cl.enqueue_copy(queue, hist_dev ,hist_np)
cl.enqueue_copy(queue, data_dev ,data_np)
queue.finish()

# cpu (numpy) baseline, both for timing and as a target value
baseline = None
with TimedBlock("numpy histogram"):
    baseline, _ = np.histogram(data_np, np.arange(257))

global_naive_prog = None
with open('global_naive.cl', 'r') as file:
    src = file.read().strip()
    global_naive_prog = cl.Program(ctx, src).build()

global_naive = global_naive_prog.global_naive
with TimedBlock("global naive histogram"):
    global_naive(queue,
                 data_np.shape,  # global work size
                 None,           # local work size
                 # args, in the order in which they appear in the kernel
                 # function's signature
                 data_dev, hist_dev,
                 np.uint32(data_np.shape[0]), np.uint32(hist_np.shape[0]))
    queue.finish()
    # ^^ wait for all commands added to the queue to finish so we can go and
    # collect their results
    # (the only command in the queue in this case is running the
    #  global_native kernel)
    # 
    # putting this in the TimedBlock block also ensures timing includes the
    # whole kernel execution is timed and not just enqueueing the kernel, as
    # the TimedBlock block will, under these circumstances, not exit until the
    # whole kernel's done
    # 
    # https://registry.khronos.org/OpenCL/sdk/3.0/docs/man/html/clFinish.html 

    # also avoid any kernel logging getting mixed with any of the subsequent logging
    sys.stdout.flush()

cl.enqueue_copy(queue, hist_np, hist_dev)
queue.finish()

if error_on_diff(baseline, hist_np, test_name="global naive histogram"):
    print("error :(")
    sys.exit(1)

# same shit different kernel
global_sectioned_prog = None
with open('global_sectioned.cl', 'r') as file:
    src = file.read().strip()
    global_sectioned_prog = cl.Program(ctx, src).build()

global_sectioned = global_sectioned_prog.global_sectioned
with TimedBlock("global sectioned histogram"):
    section_size=16
    global_work_size=(np.uint(np.ceil(data_np.shape[0]/section_size)),)
    global_sectioned(queue,
                     global_work_size,
                     None,
                     data_dev, hist_dev,
                     np.uint32(data_np.shape[0]), np.uint32(hist_np.shape[0]))
    queue.finish()
    sys.stdout.flush()


cl.enqueue_copy(queue, hist_np, hist_dev)
queue.finish()

if error_on_diff(baseline, hist_np, test_name="global naive histogram"):
    print("error :(")
    sys.exit(1)
