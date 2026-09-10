# Notice for the Professor
The gpu programming part of the project was meant to be done using CUDA, due to several reasons, including:
- lack of access to a CUDA enabled gpu
- slow wifi on the author's side so the web environments barely worked
- didn't really wanna have to pay for hardware access
- the free web ides with CUDA accesses (ie: google colab) are, quite frankly, painful to use

The author has opted to do this part using OpenCL, other technologies have have been considered but they all had problems as CUDA replacements for this project, such as
- **PCUDA**: it doesn't run on the gpu on my machine, and it requires ad hoc compilers
- **SYCL**: too dissimilar to CUDA, also requires ad hoc compilers
- **AMD HIP**: similar to CUDA and runs on the gpu, but I don't have an AMD card

OpenCL, having been modeled after CUDA, but still being available to run locally on the author's machines, has been thus chosen as the most appropriate CUDA replacement for the gpu programming part of this exam.

Due to C and C++ APIs for OpenCL being quite slow to work with and, after extensive use, been deemed unfit for experimentation, the OpenCL part has for this project had been initially developed using `pyopencl`, with plans to port it to C once it was done, due to timing constraints the C port has never been finished.

# Repository Structure
This repository is divided into 4 main sub-directories
- `openmp`: code for the openmp part of the exam
- `opencl`: code for the gpu part of this exam (done in opencl)
- `tools`: tooling which was meant to be used for both parts of the exam (was only used for the opencl part)
- `report`: report files and code/files used to generate the report files
  > NOTICE: although the code for this repository is licensed under the GNU LGPL V2.1
  > the report code is licensed under the GNU GPL V3
  > this is due to licensing in the presentation template used for the presentation, which
  > is GPL V3 licensed, and would thus be incompatible to use from LGPL V2.1 code

See `TOUR.md` file for further details on repository structure.  

I would, finally, like to state that https://www.xkcd.com/3126
