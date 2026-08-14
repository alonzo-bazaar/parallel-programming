#!/usr/bin/env python3
import numpy as np
import pyopencl as cl

from utils import (TimedBlock, error_on_diff, compile_file, round_up_to_divide,
                   kernel_test, get_curr_kernel_test_name, compare)
import os, sys

# global settings and variables
# settings
os.environ['PYOPENCL_COMPILER_OUTPUT']='1'
os.environ['PYOPENCL_CTX']='1'

# opencl global things
ctx = cl.create_some_context(interactive=False)
queue = cl.CommandQueue(ctx)

# compile kernels only once
scans_prog = compile_file('scans-ex-nihilo.cl', ctx)
ks_block_scan     = scans_prog.ks_block_scan
ks_last_elt_scan  = scans_prog.ks_last_elt_scan
ks_filling_pass   = scans_prog.ks_filling_pass

# https://documen.tician.de/pyopencl/runtime_const.html#pyopencl.mem_flags
def copy_to_cl(d_np:np.ndarray):
    global queue
    mf = cl.mem_flags
    d_cl = cl.Buffer(ctx, mf.READ_WRITE, d_np.nbytes)
    cl.enqueue_copy(queue, d_cl ,d_np)
    queue.finish()
    return d_cl

@kernel_test(name="baseline", die_on_error=False)
def compute_baseline(data:np.ndarray):
    with TimedBlock("baseline"):
        return np.cumsum(data) # welcome to the cumsum

# generic function in which to plug the elements of a  progressive scan which
# satisfy given conditions
# since this is mostly wrapper code around kernel calls and I can't be fucked to
# rewrite this for every progressive scan I test
def test_progressive_scan_whole(d_np, expected_output,
                                block_scan, last_elt_scan, filling_pass,
                                local_work_size=256):
    d_cl = copy_to_cl(d_np)
    data_size = d_np.shape[0]
    global_work_size = round_up_to_divide(data_size, local_work_size)

    data_size        = np.uint32(data_size)
    local_work_size  = np.uint32(local_work_size)
    global_work_size = np.uint32(global_work_size)

    # data will be divided in chunks with size equal to local_work_size
    # and there will therefore be a number of chunks equal to
    number_of_chunks  = np.uint32(np.ceil(data_size / local_work_size))
    single_chunk_size = np.uint32(local_work_size)

    kernel_under_test_name = get_curr_kernel_test_name()
    with TimedBlock("progressive scan" + (f": {kernel_under_test_name}"
                                          if kernel_under_test_name is not None
                                          else "")):
        block_scan(queue, (global_work_size,), (local_work_size,),
                   d_cl, data_size,
                   cl.LocalMemory(local_work_size * 4), local_work_size)
        # queue.finish()

        last_elt_scan(queue,
                      (np.uint32(round_up_to_divide(number_of_chunks, 32)),),
                      (np.uint32(round_up_to_divide(number_of_chunks, 32)),),
                      d_cl, data_size,
                      cl.LocalMemory(number_of_chunks * 4),
                      number_of_chunks,
                      single_chunk_size)
        # queue.finish()

        filling_pass(queue, (global_work_size,), (local_work_size,),
                     d_cl, data_size,
                     single_chunk_size)
        queue.finish()

    cl.enqueue_copy(queue, d_np, d_cl)
    queue.finish()
    compare(expected_output, d_np)
    return d_np

@kernel_test(name="heirarchical kogge stone", die_on_error=True)
def test_ks_whole(input_data,
                  expected_output,
                  local_work_size=256):
    test_progressive_scan_whole(input_data, expected_output,
                                local_work_size=local_work_size,

                                block_scan=ks_block_scan,
                                last_elt_scan=ks_last_elt_scan,
                                filling_pass=ks_filling_pass)


np.random.set_state(('MT19937', np.ones(624), 42))
test_sizes = [(99,     99), (99,    100), (99,    101),
              (100,    10), (100,    99), (100,   100), (100,   101),
              (101,    99), (101,   100), (101,   101),
              
              (254,   254), (255,   254), (256,   254), (257,   254),
              (254,   255), (255,   255), (256,   255), (257,   255),
              (254,   256), (255,   256), (256,   256), (257,   256),

              (1023,  254), (1024,  254), (1025,  254),
              (1023,  255), (1024,  255), (1025,  255),
              (1023,  256), (1024,  256), (1025,  256),

              (1000,  256), (10000, 256), (1000,  255), (10000, 255),
              (123,   123), (321,   123), (4321,  123), (54321, 231),

              ((2**16)-1, 256)]

for whole_size, group_size in test_sizes:
    print(f'{whole_size}, {group_size}')

    data = np.ones(whole_size, dtype=np.uint32)
    baseline_scan = compute_baseline(data)
    test_ks_whole(data,
                  expected_output=baseline_scan,
                  local_work_size=group_size)

    # data = np.random.randint(1, 1000, whole_size, dtype=np.uint32)
    # baseline_scan = compute_baseline(data)
    # test_ks_whole(data,
    #               expected_output=baseline_scan,
    #               local_work_size=group_size)
