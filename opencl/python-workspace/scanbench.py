#!/usr/bin/env python3

import numpy as np
import pyopencl as cl

from utils import TimedBlock, error_on_diff, compile_file
import os, sys

os.environ['PYOPENCL_COMPILER_OUTPUT']='1'
os.environ['PYOPENCL_CTX']='1'

ctx = cl.create_some_context(interactive=False)
queue = cl.CommandQueue(ctx)

mf = cl.mem_flags
data_length = 8123
data_np=np.ones(data_length, dtype=np.uint32)
data_dev = cl.Buffer(ctx, 0, data_np.nbytes)
cl.enqueue_copy(queue, data_dev ,data_np)
queue.finish()

with TimedBlock("baseline"):
    baseline = np.cumsum(data_np) # welcome to the cumsum

scans_prog = compile_file('scans.cl', ctx)
ungodly = scans_prog.ungodly

kogge_stone_block_scan = scans_prog.kogge_stone_block_scan
kogge_stone_last_elem_scan = scans_prog.kogge_stone_last_elem_scan
kogge_stone_filling_pass = scans_prog.kogge_stone_filling_pass

skip_ungodly=True
if not skip_ungodly:
    # prepare input for ungodly
    data_np=np.ones(data_length, dtype=np.uint32)
    cl.enqueue_copy(queue, data_dev ,data_np)
    queue.finish()
    
    # run ungodly
    with TimedBlock("ungodly"):
        ungodly(queue,
                data_np.shape,
                None,
                data_dev, np.uint32(data_np.shape[0]))
        queue.finish()
    
    cl.enqueue_copy(queue, data_np ,data_dev)
    queue.finish()
    
    error_on_diff(baseline, data_np, test_name="ungodly",
                  log_expected=True, log_actual=True, log_diff=True,
                  # plot_diff=True,
                  die_on_error=True)
    data_np = np.ones_like(data_np)
    cl.enqueue_copy(queue, data_dev ,data_np)
    queue.finish()

def round_up_to_divide(a, b):
    if a < b:
        return b
    if (a%b) == 0:
        return a
    return(a + b - (a%b))

# opencl non ho sto grandissima coordinamento tra kernel
# quindi qua famo che ne lancio 3 in questo ordine (progressive scan)
# - uno per fare lo scan di tutti i blocchi
# - uno per fare lo di tutti i primi elementi dei vari blocchi
# - e uno per farealla fine che a ogni blocco somma l'inizio dello scannato
def progressive_kogge_stone(data_np, data_dev):
    local_work_size = 50
    global_work_size = data_np.shape[0]
    global_work_size = round_up_to_divide(global_work_size, local_work_size)

    local_work_size = np.uint32(local_work_size)
    global_work_size = np.uint32(global_work_size)

    global_work_shape = (global_work_size,)
    local_work_shape = (local_work_size,)

    # first step in a progressive scan, do a scan of all chunks
    global_data_size = np.uint32(data_np.shape[0])
    kogge_stone_block_scan(queue, global_work_shape, local_work_shape,
                           data_dev, global_data_size,
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
                               data_dev,         # global data
                               global_data_size, # global data size
                               cl.LocalMemory(number_of_chunks * 4), # local data
                               number_of_chunks,                     # local data size
                               chunk_size)                           # chunk size
    queue.finish()

    kogge_stone_filling_pass(queue, global_work_shape, local_work_shape,
                             data_dev,
                             # again, we don't do any coarsening
                             # so data size is equal to work size
                             global_work_size,
                             chunk_size)
    queue.finish()

# prepare input for kogge stone
data_np=np.ones(data_length, dtype=np.uint32)
cl.enqueue_copy(queue, data_dev ,data_np)
queue.finish()

# run kogge stone
with TimedBlock("kogge stone"):
    progressive_kogge_stone(data_np, data_dev)

cl.enqueue_copy(queue, data_np ,data_dev)
queue.finish()

error_on_diff(baseline, data_np, test_name="kogge stone",
              log_expected=True, log_actual=True, log_diff=True,
              # plot_diff=True,
              die_on_error=True)
