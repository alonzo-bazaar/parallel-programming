#include<stdio.h>
#include<CL/cl.h>

#define STB_ISH_UTILS_IMPLEMENTATION
#include "utils.h"

int main() {
    cl_int err = CL_SUCCESS;

    cl_platform_id plat;
    cl_device_id dev;
    // get any gpu device
    dev = get_device(0, 0, CL_DEVICE_TYPE_GPU);
    // query the platform of the gpu device
    try_ret(clGetDeviceInfo(dev, CL_DEVICE_PLATFORM,
                            sizeof(cl_platform_id), &plat, NULL));

    // get context for device and get command queue from context
    cl_context context;
    cl_command_queue queue;
    try_par(context = clCreateContext(NULL, 1, &dev, NULL, NULL, &err));
    try_par(queue = clCreateCommandQueueWithProperties(context, dev,
                                                       NULL, &err));

    // read program code into memory
    char* program_source = NULL;
    size_t program_size = 0;
    if(!read_entire_file("hello.cl", &program_source, &program_size)) return 1;

    // create and compile program
    cl_program program = NULL;
    try_par(program =\
            clCreateProgramWithSource(context, 1,
                                      (const char **)&program_source,
                                      &program_size, &err));

    // shits itself and dies if program doesn't build correctly
    build_program(program, dev, NULL);

    // get kernel from program
    cl_kernel kernel = NULL;
    try_par(kernel = clCreateKernel(program, "vec_add", &err));

    // test data
    const size_t vectors_length = 20;
    const size_t vectors_size = vectors_length * sizeof(float);
    float* host_in1 = (float*)malloc(vectors_size);
    float* host_in2 = (float*)malloc(vectors_size);
    for(size_t i = 0; i<vectors_length; ++i) {
        host_in1[i] = (float)i;
        host_in2[i] = 10.0f;
    }

    // buffer for the results
    float* host_out = (float*)malloc(vectors_size);

    // buffers the kernel will work with
    cl_mem dev_in1, dev_in2, dev_out;
    // allocate and copy data into the first two
    try_par(dev_in1 =\
                 clCreateBuffer(context,
                                CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                vectors_size, host_in1, &err));
    try_par(dev_in2 =\
                 clCreateBuffer(context,
                                CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                vectors_size, host_in2, &err));
    // allocate the third one
    try_par(dev_out =\
            clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                           vectors_size, NULL, &err)); 

    // set kernel parameters before launching it
    try_ret(clSetKernelArg(kernel, 0, sizeof(cl_mem), &dev_out));
    try_ret(clSetKernelArg(kernel, 1, sizeof(cl_mem), &dev_in1));
    try_ret(clSetKernelArg(kernel, 2, sizeof(cl_mem), &dev_in2));

    // enqueue (ie, give command to launch) the kernel
    // why is the amount of work units passed as a pointer?
    try_ret(clEnqueueNDRangeKernel (queue, kernel, 1, NULL, &vectors_length,
                                    NULL, 0, NULL, NULL));

    try_ret(clEnqueueReadBuffer(queue, dev_out, CL_BLOCKING, 0,
                                vectors_size, (void *)host_out,
                                0, NULL, NULL));

    for(size_t i = 0; i<vectors_length; ++i)
        printf("%f ", host_out[i]);
    putchar('\n');

    // cleanup
    try_ret(clReleaseMemObject(dev_out));
    try_ret(clReleaseMemObject(dev_in2));
    free(host_in1);
    free(host_in2);
    free(host_out);
    try_ret(clReleaseMemObject(dev_in1));
    try_ret(clReleaseKernel(kernel));
    try_ret(clReleaseProgram(program));
    free(program_source);
    try_ret(clReleaseCommandQueue(queue));
    try_ret(clReleaseContext(context));

    return 0;
}
