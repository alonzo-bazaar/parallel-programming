#!/usr/bin/env python3
import numpy as np
import pyopencl as cl

from utils import TimedBlock, error_on_diff, flush, compile_file
import os, sys
os.environ['PYOPENCL_COMPILER_OUTPUT']='1'
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
mf = cl.mem_flags

data_dev = cl.Buffer(ctx, 0, data_np.nbytes)
hist_dev = cl.Buffer(ctx, 0, hist_np.nbytes)
cl.enqueue_copy(queue, hist_dev ,hist_np)
cl.enqueue_copy(queue, data_dev ,data_np)
queue.finish()

baseline = None
with TimedBlock("numpy histogram"):
    baseline, _ = np.histogram(data_np, np.arange(257))

def zero_hist_out():
    global hist_np, queue, hist_dev
    hist_np=np.zeros((256,), dtype=np.uint32)
    cl.enqueue_copy(queue, hist_dev ,hist_np)
    queue.finish()

def run_compairison(or_die=False, **kwargs):
    global basline, hist_np, queue, hist_dev
    cl.enqueue_copy(queue, hist_np, hist_dev)
    queue.finish()
    if error_on_diff(baseline, hist_np, **kwargs):
        print("error :(")
        if or_die:
            sys.exit(1)

pn_prog = compile_file('private_naive.cl', ctx)
pn_hist = pn_prog.private_naive
with TimedBlock("privatized naive histogram"):
    local_work_size = np.uint(256)
    global_work_size = np.uint(local_work_size*
                               np.ceil(data_np.shape[0]/local_work_size))
    print(f'local group size: {local_work_size}')
    print(f'global group size: {global_work_size}')
    pn_hist(queue,
            (global_work_size,),
            (local_work_size,),
            data_dev, hist_dev,
            cl.LocalMemory(256 * 4), # sizeof(uint)=4, local_hist is uint[256]
            np.uint32(data_np.shape[0]), np.uint32(hist_np.shape[0]))
    queue.finish()

run_compairison(or_die=True, test_name="privatized naive histogram")
zero_hist_out()
print("")

# one work item per data item
# but work groups may be smaller than the histogram's size
# this means a single thread/work item may be responsible for flushing more
# than one local histogram element to global memory
# since every work group still has an entire local histogram to work with
phs_prog = compile_file('private_histogram_sectioned.cl', ctx)
phs_hist = phs_prog.private_histogram_sectioned
with TimedBlock("privatized sectioned histogram"):
    local_work_size = np.uint(256)
    global_work_size = np.uint(local_work_size*
                               np.ceil(data_np.shape[0]/local_work_size))
    print(f'with local group size {local_work_size}')
    print(f'with global group size {global_work_size}')
    phs_hist(queue,
             (global_work_size,),
             (local_work_size,),
             data_dev, hist_dev,
             cl.LocalMemory(256 * 4), 
             np.uint32(data_np.shape[0]), np.uint32(hist_np.shape[0]))
    queue.finish()

run_compairison(or_die=True,
                test_name="privatized sectioned histogram", 
                log_expected=True, log_actual=True, plot_diff=True)
zero_hist_out()

# with TimedBlock("privatized sectioned histogram"):
#     section_size=16
#     global_work_size=(np.uint(np.ceil(data_np.shape[0]/section_size)),)
#     sectioned_hist(queue,
#              global_work_size,
#              None,
#              data_dev, hist_dev, LocalMemory(256 * 4), # 4 = sizeof uint
#              np.uint32(data_np.shape[0]), np.uint32(hist_np.shape[0]))
#     queue.finish()

