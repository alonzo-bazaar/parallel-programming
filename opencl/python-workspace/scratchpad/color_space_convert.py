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

csc_prog = compile_file('color_space_conversions.cl', ctx)
rgb2hsl_ker = csc_prog.rgb2hsl
hsl2rgb_ker = csc_prog.hsl2rgb

def im2arr(impath:str):
    return np.asarray(Image.open(impath))

def arrshow(arr:np.ndarray):
    plt.imshow(Image.fromarray(arr), 'RGB')
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

def arr_rgb2hsl(arr_in:np.ndarray):    
    global rgb2hsl_ker
    return arr_pixelwise_kernel(arr_in, rgb2hsl_ker)

def arr_hsl2rgb(arr_in:np.ndarray):    
    global hsl2rgb_ker
    return arr_pixelwise_kernel(arr_in, hsl2rgb_ker)


def plot_cmp_im(imgpath:str):
    rgb = im2arr(imgpath)
    hsl = arr_rgb2hsl(rgb)
    rgb_back = arr_hsl2rgb(hsl)

    fig, (ax1, ax2, ax3) = plt.subplots(3, 1)
    ax1.set_title("original rgb")
    ax1.imshow(rgb)
    ax2.set_title("converted to hsl")
    ax2.imshow(hsl)
    ax3.set_title("hsl converted back to rgb")
    ax3.imshow(rgb_back)

    plt.show()

def main(argv):
    if len(argv) == 1:
        plot_cmp_im("../images/tree_sun.jpg")
    elif len(argv) == 2:
        plot_cmp_im(argv[1])
    else:
        print("what the fuck bro")

if __name__=='__main__':
    main(sys.argv)

"""
some data to aid in debugging
hue 0   == hue max = red
hue 60  == kinda olive
hue 120 == kinda dark green
hue 120 == green
hue 180 == cyan
hue 240 == sky blue
hue 300 == purple
"""
