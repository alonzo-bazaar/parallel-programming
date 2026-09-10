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
  - optional: as a preprocessing step, an rgb image may be converted to a color space like #link("https://en.wikipedia.org/wiki/HSL_and_HSV")[hsl] for processing, then converted back to rgb after processing
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
#slide(title:"OpenCL Implementation: outline", outlined: true)[
  PyOpenCL code laid out according to following pipeline
  - host image is read from file, host histogram is initialized to all zeros
  - host image and histogram are copied over to device
  - device image is converted from rgb(a) to hsl(a) inplace
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
]
]

#slide(title:"NumPy Implementation", outlined: true)[
  - pinco pallino
  - si ruppe il nasino
  - ei era un cogliole
  - ei era un cretino
  - pinco pallino
  - si spezzo il bacino
  - lo presi a mazzate
  - con un tavolino
]

#focus-slide[Benchmarks]
#slide(title:"Benchmarks", outlined:true)[ ]
#slide(title:"Benchmarks Data")[ ]
#slide(title:"Benchmarks Code")[ ]
#slide(title:"Benchmarks Results")[ ]
#slide(title:"Benchmarks Results: flat red")[ ]
#slide(title:"Benchmarks Results: photographs")[ ]
#slide(title:"Benchmarks Results: painting")[ ]

#focus-slide[Conclusions]
#slide(title:"In Conclusion", outlined:true)[ ]
