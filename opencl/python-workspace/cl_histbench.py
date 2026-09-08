#!/usr/bin/env python3
import numpy as np
import pyopencl as cl
from PIL import Image
from sys import argv

from utils import TimedBlock, error_on_diff, compile_file, round_up_to_divide
import os, sys
os.environ['PYOPENCL_COMPILER_OUTPUT']='1'
os.environ['PYOPENCL_CTX']='1'

import matplotlib as mpl
import matplotlib.pyplot as plt

# opencl setup
ctx   = cl.create_some_context(interactive=False)
queue = cl.CommandQueue(ctx)

# kernels we're gonna use throughout the program execution
csc_prog    = compile_file('color_space_conversions.cl', ctx)
rgb2hsl_ker = csc_prog.rgb2hsl
hsl2rgb_ker = csc_prog.hsl2rgb

hists_prog    = compile_file('private_naive_histograms.cl', ctx)
lch_hist_ker  = hists_prog.xyz_z_hist

scans_prog        = compile_file('scans.cl', ctx)
ks_block_scan     = scans_prog.ks_block_scan
ks_last_elt_scan  = scans_prog.ks_last_elt_scan
filling_pass      = scans_prog.filling_pass

eqlz_prog    = compile_file('equalization.cl', ctx)
lch_eqlz_ker = eqlz_prog.xyz_z_eqlz

# https://documen.tician.de/pyopencl/runtime_const.html#pyopencl.mem_flags
def copy_to_cl(d_np:np.ndarray):
    global queue
    mf = cl.mem_flags
    d_cl = cl.Buffer(ctx,
                     mf.COPY_HOST_PTR | mf.HOST_READ_ONLY | mf.READ_WRITE,
                     hostbuf = d_np)
    return d_cl

# define the entire data pipeline first then implement the functions that
# that pipeline's gonna use
def plus_scan_inplace(d_cl,
                      data_size, local_work_size=256,
                      block_scan    = ks_block_scan,    # if None is a noop
                      last_elt_scan = ks_last_elt_scan, # if None is a noop
                      filling_pass  = filling_pass):    # if None is a noop
    global queue
    global_work_size = round_up_to_divide(data_size, local_work_size)

    data_size        = np.uint32(data_size)
    local_work_size  = np.uint32(local_work_size)
    global_work_size = np.uint32(global_work_size)

    # data will be divided in chunks with size equal to local_work_size
    # and there will therefore be a number of chunks equal to
    number_of_chunks  = np.uint32(np.ceil(data_size / local_work_size))
    single_chunk_size = np.uint32(local_work_size)

    if block_scan is not None:
        block_scan(queue, (global_work_size,), (local_work_size,),
                   d_cl, data_size,
                   cl.LocalMemory(local_work_size * 4), local_work_size)

    if last_elt_scan is not None:
        last_elt_scan(queue,
                      (np.uint32(round_up_to_divide(number_of_chunks, 32)),),
                      (np.uint32(round_up_to_divide(number_of_chunks, 32)),),
                      d_cl, data_size,
                      cl.LocalMemory(number_of_chunks * 4),
                      number_of_chunks,
                      single_chunk_size)

    if filling_pass is not None:
        filling_pass(queue, (global_work_size,), (local_work_size,),
                     d_cl, data_size,
                     single_chunk_size)

def pipeline(host_img:str):
    global ctx, queue
    global rgb2hsl_ker, hsl2rgb_ker, lch_hist_ker, lch_eqlz_ker
    
    img_width, img_height, img_nchans = host_img.shape
    img_npxls = img_width * img_height
    host_hist = np.zeros(256)

    # their gpu twins we're gonna operate on
    mf = cl.mem_flags
    dev_img = cl.Buffer(ctx,
                        mf.COPY_HOST_PTR | mf.HOST_READ_ONLY | mf.READ_WRITE,
                        host_img.nbytes,
                        hostbuf = host_img)
    dev_hist = cl.Buffer(ctx,
                         mf.COPY_HOST_PTR | mf.HOST_READ_ONLY | mf.READ_WRITE,
                         host_hist.nbytes,
                         hostbuf = host_hist)

    # first step, turn the rgb image into an hsl image
    rgb2hsl_ker(queue, (img_npxls,), None,
                dev_img, dev_img, np.uint32(img_npxls))

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
    lch_hist_ker(queue, global_work_size, local_work_size,
                 dev_img, dev_hist,
                 # array of uint32, one element per thread, 4 bytes per element
                 # we have 256 threads per block => 256 * 4 bytes per block
                 cl.LocalMemory(threads_per_block * 4), 
                 np.uint32(img_npxls), np.uint32(256))

    # plus scan of the histogram since histogram normalization requires
    # the histogram's cdf and not the histogram itself
    plus_scan_inplace(dev_hist, data_size=256)

    # normalize the l channel of the image
    lch_eqlz_ker(queue, global_work_size, local_work_size,
                 dev_img, dev_hist,
                 np.uint32(img_npxls), np.uint32(256))

    # turn the image back into rgb
    hsl2rgb_ker(queue, (img_npxls,), None,
                dev_img, dev_img, np.uint32(img_npxls))

    # send luminosity normalized rgb image back to the cpu
    target=np.empty_like(host_img)
    cl.enqueue_copy(queue, target, dev_img)

    # and we're done :D
    queue.finish()
    return Image.fromarray(target, 'RGB')

times = {}
resolutions = {}
run_times=10

def bench_norm_image(image_path:str):
    image = np.asarray(Image.open(image_path))
    image_name = image_path.split('/')[-1]
    resolutions[image_name] = image.shape
    times[image_name] = []
    for ss in [1, 2, 4, 8]:
        # image under test, derived by subsampling original image
        # array must be contiguous to be transformable into opencl buffer
        iut = np.ascontiguousarray(image[::ss,::ss,::])
        with TimedBlock(f'{image_name} with subsampling: {ss}',
                        append_time_into=times[image_name]):
            for _ in range(run_times):
                pipeline(iut)

def main():
    bench_norm_image('../images/germano.jpg')
    bench_norm_image('../images/tree_sun.jpg')
    bench_norm_image('../images/brit.jpg')
    bench_norm_image('../images/flat_red.png')
    bench_norm_image('../images/mona.jpg')
    ofp='../results/opencl.csv'
    with open(ofp, 'w') as of: 
        pkw={'file': of, 'flush': True}
        print('name,nruns,width,height,time', **pkw)
        for name in times.keys():
            (w, h, _) = resolutions[name]
            [t1, t2, t4, t8] = times[name]
            print(f'{name},{run_times},{w//1},{h//1},{t1}', **pkw)
            print(f'{name},{run_times},{w//2},{h//2},{t2}', **pkw)
            print(f'{name},{run_times},{w//4},{h//4},{t4}', **pkw)
            print(f'{name},{run_times},{w//8},{h//8},{t8}', **pkw)

if __name__=='__main__':
    main()
