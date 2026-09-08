#!/usr/bin/env python3
import os, sys
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
from utils import TimedBlock, error_on_diff, compile_file

from PIL import Image

# https://sqlpey.com/algorithm/hsl-rgb-conversion-implementations/#solution-21-numpy-vectorized-python-conversion
# hue2rgb and hsl2rgb copied and readapted from there
# rgb2hsl inspired by there

# utils
def select(*args, **kwargs) -> np.ndarray:
    # I don't like the signature of np.select
    return np.select([i[0] for i in args], [i[1] for i in args], **kwargs)
def true_like(a:np.ndarray) -> np.ndarray:
    return np.ones_like(a, dtype=np.bool)
def asstype(arrs, t):
    return [np.astype(i, t) for i in arrs]
def bound(a, by=255):
    return select((a<by, a), default=by)
def continuous(ch:np.ndarray) -> np.ndarray:
    return ch.astype(np.float32)/256
def discrete(ch:np.ndarray) -> np.ndarray:
    return bound(np.floor(ch*256)).astype(np.uint8)

# r g b \in [0, 1]
def rgb2hsl(r, g, b):
    mx = np.max([r, g, b], axis=0)
    mn = np.min([r, g, b], axis=0)
    d  = mx-mn
    l  = (mx+mn)/2

    s = select((mx==mn       , np.zeros_like(l)),
               (l>0.5        , d/(2-mx-mn)),
               (true_like(l) , d/(mx+mn)))

    h = select((mx==mn , np.zeros_like(l)),
               (mx==r  , (g-b)/d+np.where(g<b, 6, 0)),
               (mx==g  , (b-r)/d+2),
               (mx==b  , (b-r)/d+4),
               default=-1)/6

    return h, s, l

# h s l \in [0, 1]
def hsl2rgb(h, s, l):
    q = np.where(l<0.5, l*(1+s), l+s-l*s)
    p = 2*l-q

    r = hue2rgb(p, q, h+1/3)
    g = hue2rgb(p, q, h)
    b = hue2rgb(p, q, h-1/3)
    
    return r, g, b

def hue2rgb(p, q, t):
    t = t % 1.0
    return select((t<1/6 , p+(q-p)*6*t),
                  (t<1/2 , q),
                  (t<2/3 , p+(q-p)*(2/3-t)*6),
                  default=p)

# 10 volte immagine
# 10 volte 2/
# 10 volte 2/2/
# 10 volte 2/2/2/
times = {}
resolutions = {}

run_times=10
def bench_norm_image(image_path):
    rgb_image = np.asarray(Image.open(image_path))
    image_name = image_path.split('/')[-1]
    resolutions[image_name] = rgb_image.shape
    times[image_name] = []
    for ss in [1, 2, 4, 8]:
        # image under test, derived by subsampling original image
        # array made contiguous for parity with opencl impl which requires contiguity
        iut = np.ascontiguousarray(rgb_image[::ss,::ss,::])
        with TimedBlock(f'{image_name} with subsampling: {ss}',
                        append_time_into=times[image_name]):
            for _ in range(run_times):
                # get rgb
                r, g, b = np.transpose(iut, (2, 0, 1))
                r, g, b = map(continuous, [r, g, b])
                
                # get hsl
                h, s, l = rgb2hsl(r, g, b)
                # normalize l
                dl = discrete(l)
                hist_l = np.histogram(dl.flatten(), np.arange(257))[0]
                cdf_hist_l = np.cumsum(hist_l)
                dl = bound(cdf_hist_l[dl] * 256 / dl.size).astype(np.uint8)
                # get rgb back
                r, g, b = map(discrete, hsl2rgb(h, s, l))
                out = np.stack([r, g, b])

def main():
    bench_norm_image('../images/germano.jpg')
    bench_norm_image('../images/tree_sun.jpg')
    bench_norm_image('../images/brit.jpg')
    bench_norm_image('../images/flat_red.png')
    bench_norm_image('../images/mona.jpg')
    ofp='../results/numpy.csv'
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
