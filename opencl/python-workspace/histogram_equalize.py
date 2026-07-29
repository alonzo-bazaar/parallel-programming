#!/usr/bin/env python3
import numpy as np
import pyopencl as cl
from PIL import Image
from sys import argv

from utils import TimedBlock, error_on_diff, compile_file
import os, sys
os.environ['PYOPENCL_COMPILER_OUTPUT']='1'
os.environ['PYOPENCL_CTX']='1'

import matplotlib as mpl
import matplotlib.pyplot as plt

# opencl setup
ctx = cl.create_some_context(interactive=False)
queue = cl.CommandQueue(ctx)

# kernels we're gonna use throughout the program execution
csc_prog = compile_file('color_space_conversions.cl', ctx)
rgb2hsl_ker = csc_prog.rgb2hsl
hsl2rgb_ker = csc_prog.hsl2rgb

hist_prog = compile_file('private_naive.cl', ctx)
lch_hist_ker = hist_prog.xyz_z_hist

scan_prog = compile_file('scans.cl', ctx)
plus_scan_ker = scan_prog.plus_scan_inplace

# define the entire data pipeline first then implement the functions that
# that pipeline's gonna use
def pipeline(impath:str):
    global ctx, queue
    global rgb2hsl_ker, hsl2rgb_ker, lch_hist_ker, plus_scan_ker

    # our cpu side input and output
    host_img = np.asarray(Image.open(impath))
    img_width, img_height, img_nchans = host_img.size
    img_npxls = img_width * img_height
    host_hist = np.zeros(256)

    # their gpu twins we're gonna operate on
    dev_img = cl.Buffer
    dev_hist = cl.Buffer

    cl.enqueue_copy(queue, dev_img, host_img)
    cl.enqueue_copy(queue, dev_hist, host_hist)
    queue.finish()

    # first step, turn the rgb image into an hsl image
    rgb2hsl_ker(queue,       # queue on which the kernel invocation will be enqueued
                (img_npxls,) # global work size to invoke kernel with
                None,        # local work size (omitted since this kernel does no
                             #  work group specific operations, so we can pass it
                             #  None and let opencl figure it out for itself)

                # then all the parameters to pass to the kernel function
                dev_img, dev_img, np.uint32(img_npxls))
    queue.finish()

    # now that the image is in hsl compute the histogram of the l channel
    # 
    # The kernel we're using for histogram computation is purposefully
    # rather naive and midly hardcoded.
    # We have images with 8 bit channels,
    # so the histogram will be 2^8 = 256 elements,
    # if we just make the local work size 256 we can skip all of the histogram
    # partitioning computations and just use the local id of a thread within a
    # work group to determine which element of the private (and global) histogram
    # is the thread responsible for.
    # 
    # this has been measured to be about twice as fast as doing this the more
    # correct way of computing start and end indices of the region of local
    # histogram we gotta work on
    threads_per_block = 256
    number_of_blocks = np.ceil(img_npxls/threads_per_block)

    local_work_size = (np.uint32(threads_per_block),)
    global_work_size = (np.uint32(threads_per_block * number_of_blocks),)
    lch_hist_ker(queue,
                 local_work_size,
                 global_work_size,
                 dev_img, dev_hist,
                 cl.LocalMemory(threads_per_block * 4), # array of uint32
                                                        # one element per thread
                                                        # 4 bytes per element
                                                        # 256 threads per block
                                                        # 256 * 4 bytes per block
                 np.uint32(img_npxls), np.uint32(256))
    queue.finish()

    # plus scan of the histogram since histogram normalization requires
    # the histogram's cdf and not the histogram itself

    # normalize the l channel of the image

    # turn the image back into rgb

    # send luminosity normalized rgb image back to the cpu

    # and we're done :D
