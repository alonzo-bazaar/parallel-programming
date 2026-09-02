#!/usr/bin/env python3
import numpy as np
import pyopencl as cl

from utils import TimedBlock, error_on_diff, compile_file
import os, sys
os.environ['PYOPENCL_COMPILER_OUTPUT']='1'
os.environ['PYOPENCL_CTX']='1'

import matplotlib as mpl
import matplotlib.pyplot as plt

# opencl setup
ctx = cl.create_some_context()
queue = cl.CommandQueue(ctx)

np_times=[]
pn_times=[]
ps_times=[]

pn_hist_ker = compile_file('private_naive.cl', ctx).full_hist
ps_hist_ker = compile_file('private_sectioned.cl', ctx).full_hist


def compare_times(kjv_repeats:int = 1, skip_numpy:bool=False):
    global ctx, queue
    global np_times, pn_times, ps_times
    global pn_hist_ker, ps_hist_ker
    hist_np=np.zeros((256,), dtype=np.uint32)
    data_np = np.concatenate([np.fromfile('kjv.txt', dtype=np.uint8)
                              for _ in range(kjv_repeats)])

    data_dev = cl.Buffer(ctx, 0, data_np.nbytes)
    hist_dev = cl.Buffer(ctx, 0, hist_np.nbytes)
    cl.enqueue_copy(queue, hist_dev ,hist_np)
    cl.enqueue_copy(queue, data_dev ,data_np)
    queue.finish()
    
    baseline = None
    def zero_hist_out():
        hist_np=np.zeros((256,), dtype=np.uint32)
        cl.enqueue_copy(queue, hist_dev ,hist_np)
        queue.finish()

    def run_compairison(or_die=False, **kwargs):
        if skip_numpy:
            return
        cl.enqueue_copy(queue, hist_np, hist_dev)
        queue.finish()
        if error_on_diff(baseline, hist_np, **kwargs):
            print("error :(")
            if or_die:
                sys.exit(1)

    tb_kwargs={'log_start':True,
               'log_end':False,
               'trail_empty_line':False}
    if not skip_numpy:
        with TimedBlock("baseline", append_time_into=np_times, **tb_kwargs):
            baseline, _ = np.histogram(data_np, np.arange(257))

    with TimedBlock("private naive", append_time_into=pn_times, **tb_kwargs):
        local_work_size = np.uint(256)
        global_work_size = np.uint(local_work_size*
                                   np.ceil(data_np.shape[0]/local_work_size))
        pn_hist_ker(queue,
                    (global_work_size,),
                    (local_work_size,),
                    data_dev, hist_dev,
                    cl.LocalMemory(256 * 4), # sizeof(uint)=4, local_hist is uint[256]
                    np.uint32(data_np.shape[0]), np.uint32(hist_np.shape[0]))
        queue.finish()
    run_compairison()
    zero_hist_out()

    with TimedBlock("private sectioned", append_time_into=ps_times, **tb_kwargs):
        local_work_size = np.uint(256)
        global_work_size = np.uint(local_work_size*
                                   np.ceil(data_np.shape[0]/local_work_size))
        ps_hist_ker(queue,
                    (global_work_size,),
                    (local_work_size,),
                    data_dev, hist_dev,
                    cl.LocalMemory(256 * 4), 
                    np.uint32(data_np.shape[0]), np.uint32(hist_np.shape[0]))
        queue.finish()
    run_compairison()
    zero_hist_out()

def plot_results():
    if len(np_times) != 0:
        plt.plot([i for i in range(len(np_times))], np_times, label='numpy')
    plt.plot([i for i in range(len(pn_times))], pn_times, label='ocl naive')
    plt.plot([i for i in range(len(ps_times))], ps_times, label='ocl sectioned')
    plt.legend()
    plt.show()

if __name__ == '__main__':
    for i in range(1, 100):
        print(f"comparing for {i}...")
        compare_times(i, skip_numpy=True)

    plot_results()

    print(f"np_times = {np_times}")
    print(f"pn_times = {pn_times}")
    print(f"ps_times = {ps_times}")
