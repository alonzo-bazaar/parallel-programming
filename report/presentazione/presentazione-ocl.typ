#import "@preview/typslides:1.3.4": *
#show: typslides.with(
  ratio: "16-9",
  theme: "dusky",
  font: "Fira Sans",
  font-size: 20pt,
  link-style: "color",
  show-progress: true,
)

#front-slide(
  title: "Histogram Equalization in OpenCL and NumPy",
  subtitle: "Case Study in Parallelizing Image Processing",
  authors: "H.Kirollos",
  info: [#link("https://github.com/alonzo-bazaar/parallel-programming")],
)

#table-of-contents()

#focus-slide[Histogram Equalization]
#slide(title:"Histogram Equalization", outlined: true)[
  Image source: #link("https://en.wikipedia.org/wiki/Histogram_equalization")[Wikipedia]
  #linebreak()
  #cols(columns: (2fr, 2fr), gutter: 2em)[
    #image("./assets/images/uneq.jpg", height:40%)
    #image("./assets/images/eq.jpg", height:40%)
  ][
    #image("./assets/images/uneq-hist.png", height:40%)
    #image("./assets/images/eq-hist.png", height:40%)
  ]
]
#slide(title:"Procedure")[
  In general histogram equalization consists in:
  - Compute histogram of image
    - (May be of whole image, of just one channel, of every channel...)
  - Compute cdf (addition scan) of histogram
  - Compute this value for every pixel
    #image("./assets/images/formula.webp", height:20%)
  (formula from #link("https://polaris000.medium.com/histogram-equalization-c67bfa9e2a3b")[an article on medium])
  - optional: as a preprocessing step, an rgb image may be converted to a color space like #link("https://en.wikipedia.org/wiki/HSL_and_HSV")[HSL] for processing, then converted back to rgb after processing
]
#slide(title:"Our Procedure, and Parallel Patterns Therein", outlined: true)[
  what we're doing is
  - Convert the image from RGB(A) to HSL(A) (parallel pattern: *map*)
  - Compute histogram of L channel of HSL(A) image (parallel pattern: *histogram*)
  - Compute CDF of L channel (parallel pattern: *scan*)
  - Normalize L channel of image using histogram CDF (parallel pattern: *map*)
  - Turn HSL(A) image back into RGB(A) (parallel pattern: *map*)
]
#focus-slide[Implementation]
#slide(title:"Technical Choices", outlined: true)[
  - GPU version developed using Python and PyOpenCL 
    - OpenCL for running locally (issues with remote development environments)
    #linebreak()
    closest alternative to CUDA, mimics most of its API
    - Python due to development timing constraints (C port was planned but could not be made in time)
    - Slides will still use CUDA terms (thread, thread block, grid, ...) instead of the corresponding OpenCL terms (work item, work group, ndrange, ...)
  - CPU version developed using NumPy
    - Fairer compairison with GPU Python code than doing it in C or C++
    (any potential overhead from python runtime will be present in both the parallel and the sequential version)
]
#slide(title:"OpenCL Implementation", outlined: true)[
  PyOpenCL code laid out according to following pipeline
  - host image is read from file, host histogram is initialized to all zeros
  - host image and histogram are copied over to device
  - device image is converted from RGB(A) to HSL(A) inplace
  - histogram of L channel in device image computed into device histogram
  - compute addition scan of device histogram inplace
  - device image L channel normalized inplace
]
#slide(title:"OpenCL Implementation: kernels used")[
  - *Color space conversion*:
    - every thread acts on one pixel of the image, changing it inplace
    - 1d grid, image is treated as a flat array of pixels as no 2d specific behaviour is required due to the pixelwise nature of the operation
]
#slide(title:"OpenCL Implementation: histogram kernel")[
  - every thread acts on one pixel of the image
  - thread blocks hold private partial histograms in local memory to avoid contention in global memory
  - the first 256 threads in a block flush the partial sums to the global sum, after a barrier ensuring all elements have been counted in block local partial sum
  - contention with multiple threads needing to increment values in the same array (histogram) handled through atomic operations
  - 1d array, image treated as flat array of pixels, same reasoning as previous slide
]

#slide(title:"OpenCL Implementation: histogram kernel code")[
```c
__kernel void full_hist(const __global uchar* input,
                        __global uint* global_hist,
                        __local uint* local_hist,
                        const uint input_size,
                        const uint hist_size) {
    const uint gi = get_global_id(0);
    const uint li = get_local_id(0);
    if(li < hist_size) local_hist[li] = 0;
    barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);
    if(gi<input_size)
        atomic_add(&local_hist[input[gi]], 1);
    barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);
    if(li < hist_size)
        atomic_add(&global_hist[li], local_hist[li]);
}
```
]
#slide(title:"OpenCL Implementation: addition scan kernel")[
  - 2 level heirerachical scan, handles up to $"max_block_size"^(2)$ elements
  - kogge-stone and brent-kung versions have been made
  - brent-kung used for both levels, though given input size (256 elements) the choice was arbitrary and had no performance impact
  - OpenCL has no mechanism to have grid level syncrhonization, heirarchical scan threfore implmeneted as 3 kernels
  - (code in next slide for one level of ks since bk code didn't fit) 
]
#slide(title:"OpenCL Implementation: addition scan code")[
#text(size: 18pt)[
```c
__kernel void ks_block_scan(__global uint* global_data,
                            const uint global_data_size,
                            __local   uint* local_data,
                            const uint local_data_size) {
    const uint global_idx = get_global_id(0);
    const uint local_idx  = get_local_id(0);
    const bool is_active  = (global_idx < global_data_size);
    local_data[local_idx]=is_active?global_data[global_idx]:0;
    barrier(CLK_LOCAL_MEM_FENCE);
    for(uint stride = 1; stride < local_data_size; stride*=2) {
        const uint tmp=(local_idx>=stride)?local_data[local_idx-stride]:0;
        barrier(CLK_LOCAL_MEM_FENCE);
        if (local_idx >= stride) local_data[local_idx] += tmp;
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    if(is_active) global_data[global_idx] = local_data[local_idx];
}
```
]]

#slide(title:"NumPy Implementation", outlined: true)[
  NumPy code laid out according to following pipeline
  - image is split into R, G, B (and A) channels (transposition then splitting)
  - R, G, and B channels are "undiscretized" from `np.uint8` into `np.float`
  - continuous R, G, and B channels are used to compute continuous H, S, and L channels (not inplace)
  - L channel undergoes histogram normalization (requires discretization as an intermediate step)
  - H, S, and normalized L channel are turned back into R, G, and B channels
  - the normalized R, G, and B channels are used (together with the A channel) to construct the normalized image (requires another transposition)
  This is a different set of operations from the PyOpenCL code but porting the degree of index fiddling done in OpenCL to NumPy proved problematic, requiring the two transpositions.
]
#slide(title:"NumPy Implementation: code")[
```python
def continuous(ch:np.ndarray) -> np.ndarray:
    return ch.astype(np.float32)/256
def discrete(ch:np.ndarray) -> np.ndarray:
    return bound(np.floor(ch*256), by=255).astype(np.uint8)

# ...

r, g, b = np.transpose(image, (2, 0, 1))
r, g, b = map(continuous, [r, g, b])
h, s, l = rgb2hsl(r, g, b)
dl = discrete(l)
hist_l = np.histogram(dl.flatten(), np.arange(257))[0]
cdf_hist_l = np.cumsum(hist_l)
dl = bound(cdf_hist_l[dl] * 256 / dl.size).astype(np.uint8)
r, g, b = map(discrete, hsl2rgb(h, s, l))
out = np.stack([r, g, b])
```
]

#focus-slide[Benchmarks]
#slide(title:"Benchmarks", outlined:true)[
  The PyOpenCL and the numpy versions have been tested on a small set of heterogeneous images to see how much would they take to equalize the image 10 times #linebreak()
  The test images used are:
  #cols(columns: (3fr, 2fr, 1fr, 3fr, 1fr), gutter: 2em)[
    #image("assets/images/germano.jpg", height:30%)
 ][ #image("assets/images/flat_red.png", height:30%)
 ][ #image("assets/images/small-mona.jpg", height:30%)
 ][ #image("assets/images/tree_sun.jpg", height:30%)
 ][ #image("assets/images/brit.jpg", height:30%) ]
]
#slide(title:"Benchmarks Code")[ ]
#slide(title:"Benchmarks Results")[
  Time taken to equalize 10 image times 
#text(size: 15pt)[
  #cols(columns: (1fr, 1fr, 1fr, 1fr, 1fr), gutter: 0em)[
    #image("assets/images/germano.jpg", height:20%) #linebreak()
    - Image Size: 421x748
    - NumPy: 0.659s
    - PyOpenCL: 0.023s
    - Speedup: *28.64*
 ][ #image("assets/images/flat_red.png", height:20%)
    - Image Size: 100x100
    - NumPy: 0.021s
    - PyOpenCL: 0.012s
    - Speedup: *1.642*
 ][ #image("assets/images/small-mona.jpg", height:20%)
    - Image Size: 11146x7479
    - NumPy: 223.8s
    - PyOpenCL: 4.863s
    - Speedup: *46.03*
 ][ #image("assets/images/tree_sun.jpg", height:20%)
    - Image Size: 796x1200
    - NumPy: 2.016s
    - PyOpenCL: 0.041s
    - Speedup: *48.13*
 ][ #image("assets/images/brit.jpg", height:20%)
    - Image Size: 800x528
    - NumPy: 0.835s
    - PyOpenCL: 0.025s
    - Speedup: *33.35*
 ]]]

#focus-slide[Conclusions]
#slide(title:"In Conclusion", outlined:true)[
  The PyOpenCL version is significantly faster in most cases. With a speedup 45 for larger images, and around 30 for smaller images. #linebreak()

  The main outliar is the *1.642* speedup for the small flat red image, this is likely due to the image being very small, leading to high relative cost of sending the data over to gpu, being flat, there will also be high contention in the histogram kernel with all threads writing to the same address, leading to further loss of performance in the parallel version.
]
