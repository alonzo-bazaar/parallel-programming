#!/usr/bin/env python3
import numpy as np
import pyopencl as cl
from PIL import Image

from utils import TimedBlock, error_on_diff, compile_file
import os, sys
os.environ['PYOPENCL_COMPILER_OUTPUT']='1'
os.environ['PYOPENCL_CTX']='1'

import matplotlib as mpl
import matplotlib.pyplot as plt

# opencl setup
ctx = cl.create_some_context(interactive=False)
queue = cl.CommandQueue(ctx)

hist_prog = compile_file('private_naive.cl', ctx)
hist_ker = hist_prog.private_naive

csc_prog = compile_file('color_space_conversions.cl', ctx)
rgb2hsv_ker = csc_prog.rgb2hsv
hsv2rgb_ker = csc_prog.hsv2rgb

def im2arr(impath:str):
    return np.asarray(Image.open(impath))

def arrshow(arr:np.ndarray):
    plt.imshow(Image.fromarray(arr))
    plt.show()

def arr_pixelwise_kernel(arr_in:np.ndarray, ker:cl.Kernel):    
    global ctx, queue
    nrows, ncols, nchns = arr_in.shape
    worksize=nrows*ncols

    arr_out = np.zeros_like(arr_in)
    dev_in =  cl.Buffer(ctx, 0, arr_in.nbytes)
    dev_out = cl.Buffer(ctx, 0, arr_out.nbytes)
    cl.enqueue_copy(queue, dev_in,  arr_in)
    cl.enqueue_copy(queue, dev_out, arr_out)
    queue.finish()

    ker(queue, (worksize,), None,
        dev_in, dev_out, np.uint32(worksize))
    queue.finish()

    cl.enqueue_copy(queue, arr_out, dev_out)
    return arr_out

def arr_rgb2hsv(arr_in:np.ndarray):    
    global rgb2hsv_ker
    return arr_pixelwise_kernel(arr_in, rgb2hsv_ker)

def arr_hsv2rgb(arr_in:np.ndarray):    
    global hsv2rgb_ker
    return arr_pixelwise_kernel(arr_in, hsv2rgb_ker)


def plot_cmp_im(imgpath:str):
    rgb = im2arr(imgpath)
    hsv = arr_rgb2hsv(rgb)
    rgb_back = arr_hsv2rgb(hsv)

    fig, (ax1, ax2, ax3) = plt.subplots(3, 1)
    ax1.set_title("original rgb")
    ax1.imshow(rgb)
    ax2.set_title("converted to hsv")
    ax2.imshow(hsv)
    ax3.set_title("hsv converted back to rgb")
    ax3.imshow(rgb_back)

    plt.show()

plot_cmp_im("../images/tree_sun.jpg")
