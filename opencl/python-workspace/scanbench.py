#!/usr/bin/env python3

import numpy as np
import pyopencl as cl

from utils import TimedBlock, error_on_diff, compile_file
import os, sys

os.environ['PYOPENCL_COMPILER_OUTPUT']='1'
os.environ['PYOPENCL_CTX']='1'

ctx = cl.create_some_context()
queue = cl.CommandQueue(ctx)

mf = cl.mem_flags
data_np=np.ones(1024, dtype=np.uint32)
data_dev = cl.Buffer(ctx, 0, data_np.nbytes)

with TimedBlock("baseline"):
    baseline = np.cumsum(data_np) # welcome to the cumsum

cl.enqueue_copy(queue, data_dev ,data_np)
queue.finish()

scans_prog = compile_file('scans.cl', ctx)
ungodly = scans_prog.ungodly

kogge_stone_block_scan = scans_prog.kogge_stone_block_scan
kogge_stone_first_elem_scan = scans_prog.kogge_stone_first_elem_scan
kogge_stone_filling_scan = scans_prog.kogge_stone_filling_scan

# with TimedBlock("ungodly"):
#     ungodly(queue,
#             data_np.shape,
#             None,
#             data_dev, np.uint32(data_np.shape[0]))
#     queue.finish()
# 
# cl.enqueue_copy(queue, data_np ,data_dev)
# queue.finish()
# 
# error_on_diff(baseline, data_np, test_name="ungodly",
#               log_expected=True, log_actual=True, die_on_error=True)
# data_np = np.ones_like(data_np)
# cl.enqueue_copy(queue, data_dev ,data_np)
# queue.finish()

# opencl non ho sto grandissima coordinamento tra kernel
# quindi qua famo che ne lancio 3 in questo ordine (progressive scan)
# - uno per fare lo scan di tutti i blocchi
# - uno per fare lo di tutti i primi elementi dei vari blocchi
# - e uno per farealla fine che a ogni blocco somma l'inizio dello scannato
def kogge_stone(data_np, data_dev):
    work_group_size = 256
    global_work_size = data_np.shape[0]
    global_work_size += (work_group_size - global_work_size%work_group_size)

    work_group_size = np.uint32(work_group_size)
    global_work_size = np.uint32(global_work_size)

    global_work_shape = (global_work_size,)
    local_work_shape = (local_work_size,)

    kogge_stone_block_scan(queue, global_work_shape, local_work_shape,
                           data_dev, data_np.shape[0],
                           cl.LocalMemory(local_work_size * 4), local_work_size)
    queue.finish()

    kogge_stone_first_elem_scan(queue, local_work_shape, local_work_shape,
                                data.dev, data_np.shape[0], local_work_shape)
    queue.finish()

    kogge_stone_filling_scan(queue, global_work_shape, local_work_shape,
                                data.dev, data_np.shape[0], local_work_shape)
    queue.finish()

with TimedBlock("kogge stone"):

cl.enqueue_copy(queue, data_np ,data_dev)
queue.finish()

error_on_diff(baseline, data_np, test_name="kogge stone",
              log_expected=True, log_actual=True, die_on_error=True)
