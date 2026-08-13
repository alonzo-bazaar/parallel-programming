#!/usr/bin/env python3
import numpy as np
import pyopencl as cl
mf = cl.mem_flags

from utils import TimedBlock, error_on_diff, compile_file, round_up_to_divide
import os, sys, copy, functools

# global settings and variables
# settings
os.environ['PYOPENCL_COMPILER_OUTPUT']='1'
os.environ['PYOPENCL_CTX']='1'

# opencl global things
ctx = cl.create_some_context(interactive=False)
queue = cl.CommandQueue(ctx)

# compile kernels only once
scans_prog = compile_file('scans.cl', ctx)
ungodly = scans_prog.ungodly
kogge_stone_block_scan = scans_prog.kogge_stone_block_scan
kogge_stone_last_elem_scan = scans_prog.kogge_stone_last_elem_scan
kogge_stone_filling_pass = scans_prog.kogge_stone_filling_pass

curr_kernel_test_name=None
curr_kernel_compare_kwargs = {
    'log_expected' :True,
    'log_actual'   :True,
    'log_diff'     :True,
    #'plot_diff'    :True,
    'die_on_error' :True,
}

# https://realpython.com/primer-on-python-decorators/#finding-yourself
# https://realpython.com/primer-on-python-decorators/#defining-decorators-with-arguments
def kernel_test(name:str, **kt_kwargs):
    def kernel_test_decorator(fn):
        @functools.wraps(fn)
        def wrapped_kernel_test(*wkt_args, **wkt_kwargs):
            global curr_kernel_test_name, curr_kernel_compare_kwargs

            # backup old global settings
            old_kernel_test_name = copy.copy(curr_kernel_test_name)
            old_kernel_compare_kwargs = copy.copy(curr_kernel_compare_kwargs)

            # update global settings to reflect configuration parameters
            curr_kernel_test_name = name
            for k in kt_kwargs.keys():
                curr_kernel_compare_kwargs[k] = kt_kwargs[k]

            # call wrapped function in this updated global environment
            res = fn(*wkt_args, **wkt_kwargs)

            # restore old global state
            curr_kernel_test_name = old_kernel_test_name
            curr_kernel_compare_kwargs = old_kernel_compare_kwargs

            return res
        return wrapped_kernel_test
    return kernel_test_decorator

def compare(expected:np.ndarray, actual:np.ndarray):
    error_on_diff(expected, actual, test_name=curr_kernel_test_name,
                  **curr_kernel_compare_kwargs)

def np_cl_ones(length:int):
    global queue, ctx
    data_np=np.ones(length, dtype=np.uint32)
    data_dev = cl.Buffer(ctx, 0, data_np.nbytes)
    cl.enqueue_copy(queue, data_dev ,data_np)
    queue.finish()
    return data_np, data_dev

def np_cl_ones_like(baseline:np.ndarray):
    return np_cl_ones(baseline.shape[0])

@kernel_test(name="baseline", die_on_error=False)
def compute_baseline(length:int):
    d_np = np.ones(length)
    with TimedBlock("baseline"):
        res = np.cumsum(d_np) # welcome to the cumsum
    return res

@kernel_test(name="ungodly", die_on_error=False)
def test_ungodly(baseline:np.ndarray):
    d_np, d_cl = np_cl_ones_like(baseline)
    with TimedBlock("ungodly"):
        ungodly(queue,
                d_np.shape,
                None,
                d_cl, np.uint32(d_np.shape[0]))
        queue.finish()
    cl.enqueue_copy(queue, d_np ,d_cl)
    queue.finish()
    compare(d_np, baseline)
    return d_np

# opencl non ho sto grandissima coordinamento tra kernel
# quindi qua famo che ne lancio 3 in questo ordine (progressive scan)
# - uno per fare lo scan di tutti i blocchi
# - uno per fare lo di tutti i primi elementi dei vari blocchi
# - e uno per farealla fine che a ogni blocco somma l'inizio dello scannato
def progressive_kogge_stone(d_np:np.ndarray, d_cl:cl.Buffer):
    local_work_size = 50
    global_work_size = d_np.shape[0]
    global_work_size = round_up_to_divide(global_work_size, local_work_size)

    local_work_size = np.uint32(local_work_size)
    global_work_size = np.uint32(global_work_size)

    global_work_shape = (global_work_size,)
    local_work_shape = (local_work_size,)

    # first step in a progressive scan, do a scan of all chunks
    global_data_size = np.uint32(d_np.shape[0])
    kogge_stone_block_scan(queue, global_work_shape, local_work_shape,
                           d_cl, global_data_size,
                           cl.LocalMemory(local_work_size * 4),
                           local_work_size)
    queue.finish()

    # then do a scan considering only the last elements of every chunk 
    # (clojure threading macros be like)
    chunk_size = local_work_size # we don't do any coarsening, so the chunks are
                                 # the same size as the work group, and every 
                                 # work item in a work group handles one
                                 # element of the input array

    number_of_chunks = np.ceil(global_work_size/local_work_size)
    number_of_chunks = round_up_to_divide(number_of_chunks, 32)
    number_of_chunks = np.uint32(number_of_chunks)
    kogge_stone_last_elem_scan(queue,
                               (number_of_chunks,), (number_of_chunks,),
                               d_cl,         # global data
                               global_data_size, # global data size
                               cl.LocalMemory(number_of_chunks * 4), # local data
                               number_of_chunks,                     # local data size
                               chunk_size)                           # chunk size
    queue.finish()
    return

    kogge_stone_filling_pass(queue, global_work_shape, local_work_shape,
                             d_cl,
                             # again, we don't do any coarsening
                             # so data size is equal to work size
                             global_work_size,
                             chunk_size)
    queue.finish()

@kernel_test(name="progressive kogge stone", die_on_error=False)
def test_progressive(baseline:np.ndarray):
    d_np, d_cl = np_cl_ones_like(baseline)
    with TimedBlock("kogge stone"):
        progressive_kogge_stone(d_np, d_cl)
    cl.enqueue_copy(queue, d_np ,d_cl)
    queue.finish()
    # compare(baseline, d_np)
    return d_np

def main():
    data_length = 8123
    # baseline = compute_baseline(data_length)
    baseline = np.ones(data_length)
    # test_ungodly(baseline)
    tp = test_progressive(baseline)

    print(test_progressive(baseline))

if __name__=='__main__':
    main()

