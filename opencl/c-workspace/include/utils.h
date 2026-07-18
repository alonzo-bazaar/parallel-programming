#pragma once

#include<stdbool.h>
#include<stdlib.h>  // malloc, free
#include<stdio.h>   // printf
#include<errno.h>   // errno
#include<string.h>  // strerror
#include<time.h>

#include<CL/cl.h>

// some extra error codes that cl_util_print_error expects to exist
#define CL_UTIL_INDEX_OUT_OF_RANGE -2000
#define CL_UTIL_DEVICE_NOT_INTEROPERABLE -2001
#define CL_UTIL_FILE_OPERATION_ERROR -2002

// given that every goddamn function in the opencl api has an error return
// and that if() return 1; if() return 1; if () return 1; if() return 1;...
// gets old very quickly, we just have a macro for that
// well, two macros
// opencl functions can return an error either as their return value (try_ret)
// or they can receive a pointer to an error return and maybe set that to their
// exit status (try_par_err)

// given that it's rather common to just have one cl_int err in the function
// and you just pass that as a pointer to every function that takes a
// &cl_int err
// I have also defined try_par as a convenience shorthand for try_par_err

// to simplify error handling we don't have any error labels we goto, like
// the ones in error.h, we just fucking die

// try for functions that take an error parameter
// for the sake of brevity it assumes there's a cl_int err somewhere
// and that the error parameter in the function call was &err
// if your error return parameter int isn't called err, use try_par_err

#define try_par(call) do{                                       \
        call;                                                   \
        if (err != CL_SUCCESS) {                                \
            cl_util_print_error(err);                           \
            fprintf(stderr, "on line %d, in file %s\n%s\n",     \
                    __LINE__,                                   \
                    __FILE__, #call);                           \
            exit(1);                                            \
        }                                                       \
    } while(0)

// same as try_par but accepts an extra parameter for where the error return
// is going to be
#define try_par_err(call, err) do{                              \
        call;                                                   \
        if (err != CL_SUCCESS) {                                \
            cl_util_print_error(err);                           \
            fprintf(stderr, "on line %d, in file %s\n%s\n",     \
                    __LINE__,                                   \
                    __FILE__, #call);                           \
            exit(1);                                            \
        }                                                       \
    } while(0)


// try for functions that return an error
#define try_ret(call) do{                                       \
        cl_int err = call;                                      \
        if (err != CL_SUCCESS) {                                \
            cl_util_print_error(err);                           \
            fprintf(stderr, "on line %d, in file %s\n%s\n",     \
                    __LINE__,                                   \
                    __FILE__, #call);                           \
            exit(1);                                            \
        }                                                       \
    } while(0)

// or just die for reasons unreated to opencl
// two separate macros so we don't get a syntax error for empty variadic args
// there is __VA_OPT__ but that's c++20 and I don't know if I wanna depend on that
#define die(msg) do{                                            \
        fprintf(stderr, "FATAL ERROR on line %d, in file %s\n", \
                __LINE__,                                       \
                __FILE__);                                      \
        fprintf(stderr, msg "\n");                              \
        exit(1);                                                \
    } while(0)

#define die_fmt(fmt, args...) do{                               \
        fprintf(stderr, "FATAL ERROR on line %d, in file %s\n", \
                __LINE__,                                       \
                __FILE__);                                      \
        fprintf(stderr, fmt "\n", args);                        \
        exit(1);                                                \
    } while(0)


// to read an entire opencl program
bool read_entire_file(const char *path, char** into, size_t* file_size);

void cl_util_print_error(const cl_int error);

// to build an opencl program and, if build fails, show compilation errors
cl_int build_program(const cl_program pr, const cl_device_id dev,
                     const char *const opt);

cl_device_id get_device(const cl_uint plat_id,
                        const cl_uint dev_id,
                        const cl_device_type type);

#ifdef STB_ISH_UTILS_IMPLEMENTATION
#include<stdio.h>  // for file stuff
#include<CL/cl.h>  // for compilation stuff

// stackoverflow was getting too much into the weeds of platform specific shit
// so I just stole (and readapted) this from nob.h

// https://github.com/tsoding/nob.h/blob/2210dccb978603e0deb4acd08ae3448b00f160f5/nob.h#L2429C1-L2464C2

// returns true success and false on failure
bool read_entire_file(const char *path, char** into, size_t* file_size) {
    bool result = true;

    FILE *f = fopen(path, "rb");
    long long m = 0;
    if (f == NULL || fseek(f, 0, SEEK_END) < 0) {
        result = false;
        goto defer;
    }

#ifndef _WIN32
    m = ftell(f);
#else
    m = _telli64(_fileno(f));
#endif

    if (m < 0 || fseek(f, 0, SEEK_SET)) {
        result = false;
        goto defer;
    }

    // read file into null terminated string
    *into = (char*)malloc(m+1 * sizeof(char));
    fread((void*)*into, m, 1, f);
    (*into)[m] = '\0';

    if (ferror(f)) {
        // ferror does not set errno.
        // So the error reporting in defer is not correct in this case.
        result = false;
        goto defer;
    }

 defer:
    if (!result)
        fprintf(stderr, "Could not read file %s: %s", path, strerror(errno));
    else
        // returned size will be equivalent to strlen, so without the '\0'
        *file_size = m;
    if (f)
        fclose(f);
    return result;
}

// taken from opencl's sdk utils, adapted from cl_util_build_program
// build program and show log if build is not successful
cl_int build_program(const cl_program pr, const cl_device_id dev,
                     const char *const opt) {
    cl_int err = clBuildProgram(pr, 1, &dev, opt, NULL, NULL);
    if (err != CL_SUCCESS) {
        // no error handling here as error from build program is more valuable
        char *program_log;
        size_t log_size = 0;
        clGetProgramBuildInfo(pr, dev, CL_PROGRAM_BUILD_LOG, 0, NULL,
                              &log_size);
        if ((program_log = (char *)malloc((log_size + 1) * sizeof(char)))) {
            clGetProgramBuildInfo(pr, dev, CL_PROGRAM_BUILD_LOG, log_size,
                                  program_log, NULL);
            program_log[log_size] = '\0';
            printf("Build log is:\n\n%s\nOptions:\n%s\n\n", program_log, opt);
            free(program_log);
        }
    }
    return err;
}

// taken from opencl's sdk utils, adapted from cl_util_get_device
// get device given platform id, device id, and device type
cl_device_id get_device(const cl_uint plat_id,
                        const cl_uint dev_id,
                        const cl_device_type type) {
    cl_platform_id *platforms;
    cl_uint num_platforms = 0;
    cl_device_id *devices;
    cl_uint num_devices = 0;

    // we must get the platform to look for the device in there
    // start by getting the number of platforms
    try_ret(clGetPlatformIDs(0, NULL, &num_platforms));

    if (plat_id >= num_platforms)
        die_fmt("invalid platform id, "
                "platform number [%u] is greater than number of platforms [%u]",
                plat_id, num_platforms);

    // allocate a buffer for that many platforms
    platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * num_platforms);
    if(platforms == NULL) die("most likely we're out of memory");

    // populate that vector with IDs for the available platforms
    try_ret(clGetPlatformIDs(num_platforms, platforms, NULL));

    // we got the platform, now we look for the device on that platform
    // get number of devices on that platform
    try_ret(clGetDeviceIDs(platforms[plat_id], type, 0, NULL, &num_devices));

    if (dev_id >= num_devices)
        die_fmt("invalid device id, "
                "device number [%u] is greater than number of devices for platform [%u]",
                plat_id, num_platforms);

    // allocate a vector for that many devices
    devices = (cl_device_id *)malloc(sizeof(cl_device_id) * num_devices);
    if(devices == NULL) die("most likely we're out of memory");

    // populate that vector with IDs for the available devices
    try_ret(clGetDeviceIDs(platforms[plat_id], type, num_devices, devices, NULL));

    cl_device_id result = devices[dev_id];
    
    free(platforms);
    free(devices);

    return result;
}

// taken from opencl's sdk utils
// prints out description of error given its error code
void cl_util_print_error(const cl_int error) {
    switch (error) {
    case CL_SUCCESS: break;
    case CL_DEVICE_NOT_FOUND:
        fprintf(stderr, "\nError: CL_DEVICE_NOT_FOUND\n");
        break;
    case CL_DEVICE_NOT_AVAILABLE:
        fprintf(stderr, "\nError: CL_DEVICE_NOT_AVAILABLE\n");
        break;
    case CL_COMPILER_NOT_AVAILABLE:
        fprintf(stderr, "\nError: CL_COMPILER_NOT_AVAILABLE\n");
        break;
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:
        fprintf(stderr, "\nError: CL_MEM_OBJECT_ALLOCATION_FAILURE\n");
        break;
    case CL_OUT_OF_RESOURCES:
        fprintf(stderr, "\nError: CL_OUT_OF_RESOURCES\n");
        break;
    case CL_OUT_OF_HOST_MEMORY:
        fprintf(stderr, "\nError: CL_OUT_OF_HOST_MEMORY\n");
        break;
    case CL_PROFILING_INFO_NOT_AVAILABLE:
        fprintf(stderr, "\nError: CL_PROFILING_INFO_NOT_AVAILABLE\n");
        break;
    case CL_MEM_COPY_OVERLAP:
        fprintf(stderr, "\nError: CL_MEM_COPY_OVERLAP\n");
        break;
    case CL_IMAGE_FORMAT_MISMATCH:
        fprintf(stderr, "\nError: CL_IMAGE_FORMAT_MISMATCH\n");
        break;
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:
        fprintf(stderr, "\nError: CL_IMAGE_FORMAT_NOT_SUPPORTED\n");
        break;
    case CL_BUILD_PROGRAM_FAILURE:
        fprintf(stderr, "\nError: CL_BUILD_PROGRAM_FAILURE\n");
        break;
    case CL_MAP_FAILURE:
        fprintf(stderr, "\nError: CL_MAP_FAILURE\n");
        break;
#ifdef CL_VERSION_1_1
    case CL_MISALIGNED_SUB_BUFFER_OFFSET:
        fprintf(stderr, "\nError: CL_MISALIGNED_SUB_BUFFER_OFFSET\n");
        break;
    case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
        fprintf(stderr,
                "\nError: CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST\n");
        break;
#endif
#ifdef CL_VERSION_1_2
    case CL_COMPILE_PROGRAM_FAILURE:
        fprintf(stderr, "\nError: CL_COMPILE_PROGRAM_FAILURE\n");
        break;
    case CL_LINKER_NOT_AVAILABLE:
        fprintf(stderr, "\nError: CL_LINKER_NOT_AVAILABLE\n");
        break;
    case CL_LINK_PROGRAM_FAILURE:
        fprintf(stderr, "\nError: CL_LINK_PROGRAM_FAILURE\n");
        break;
    case CL_DEVICE_PARTITION_FAILED:
        fprintf(stderr, "\nError: CL_DEVICE_PARTITION_FAILED\n");
        break;
    case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:
        fprintf(stderr, "\nError: CL_KERNEL_ARG_INFO_NOT_AVAILABLE\n");
        break;
#endif
    case CL_INVALID_VALUE:
        fprintf(stderr, "\nError: CL_INVALID_VALUE\n");
        break;
    case CL_INVALID_DEVICE_TYPE:
        fprintf(stderr, "\nError: CL_INVALID_DEVICE_TYPE\n");
        break;
    case CL_INVALID_PLATFORM:
        fprintf(stderr, "\nError: CL_INVALID_PLATFORM\n");
        break;
    case CL_INVALID_DEVICE:
        fprintf(stderr, "\nError: CL_INVALID_DEVICE\n");
        break;
    case CL_INVALID_CONTEXT:
        fprintf(stderr, "\nError: CL_INVALID_CONTEXT\n");
        break;
    case CL_INVALID_QUEUE_PROPERTIES:
        fprintf(stderr, "\nError: CL_INVALID_QUEUE_PROPERTIES\n");
        break;
    case CL_INVALID_COMMAND_QUEUE:
        fprintf(stderr, "\nError: CL_INVALID_COMMAND_QUEUE\n");
        break;
    case CL_INVALID_HOST_PTR:
        fprintf(stderr, "\nError: CL_INVALID_HOST_PTR\n");
        break;
    case CL_INVALID_MEM_OBJECT:
        fprintf(stderr, "\nError: CL_INVALID_MEM_OBJECT\n");
        break;
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
        fprintf(stderr, "\nError: CL_INVALID_IMAGE_FORMAT_DESCRIPTOR\n");
        break;
    case CL_INVALID_IMAGE_SIZE:
        fprintf(stderr, "\nError: CL_INVALID_IMAGE_SIZE\n");
        break;
    case CL_INVALID_SAMPLER:
        fprintf(stderr, "\nError: CL_INVALID_SAMPLER\n");
        break;
    case CL_INVALID_BINARY:
        fprintf(stderr, "\nError: CL_INVALID_BINARY\n");
        break;
    case CL_INVALID_BUILD_OPTIONS:
        fprintf(stderr, "\nError: CL_INVALID_BUILD_OPTIONS\n");
        break;
    case CL_INVALID_PROGRAM:
        fprintf(stderr, "\nError: CL_INVALID_PROGRAM\n");
        break;
    case CL_INVALID_PROGRAM_EXECUTABLE:
        fprintf(stderr, "\nError: CL_INVALID_PROGRAM_EXECUTABLE\n");
        break;
    case CL_INVALID_KERNEL_NAME:
        fprintf(stderr, "\nError: CL_INVALID_KERNEL_NAME\n");
        break;
    case CL_INVALID_KERNEL_DEFINITION:
        fprintf(stderr, "\nError: CL_INVALID_KERNEL_DEFINITION\n");
        break;
    case CL_INVALID_KERNEL:
        fprintf(stderr, "\nError: CL_INVALID_KERNEL\n");
        break;
    case CL_INVALID_ARG_INDEX:
        fprintf(stderr, "\nError: CL_INVALID_ARG_INDEX\n");
        break;
    case CL_INVALID_ARG_VALUE:
        fprintf(stderr, "\nError: CL_INVALID_ARG_VALUE\n");
        break;
    case CL_INVALID_ARG_SIZE:
        fprintf(stderr, "\nError: CL_INVALID_ARG_SIZE\n");
        break;
    case CL_INVALID_KERNEL_ARGS:
        fprintf(stderr, "\nError: CL_INVALID_KERNEL_ARGS\n");
        break;
    case CL_INVALID_WORK_DIMENSION:
        fprintf(stderr, "\nError: CL_INVALID_WORK_DIMENSION\n");
        break;
    case CL_INVALID_WORK_GROUP_SIZE:
        fprintf(stderr, "\nError: CL_INVALID_WORK_GROUP_SIZE\n");
        break;
    case CL_INVALID_WORK_ITEM_SIZE:
        fprintf(stderr, "\nError: CL_INVALID_WORK_ITEM_SIZE\n");
        break;
    case CL_INVALID_GLOBAL_OFFSET:
        fprintf(stderr, "\nError: CL_INVALID_GLOBAL_OFFSET\n");
        break;
    case CL_INVALID_EVENT_WAIT_LIST:
        fprintf(stderr, "\nError: CL_INVALID_EVENT_WAIT_LIST\n");
        break;
    case CL_INVALID_EVENT:
        fprintf(stderr, "\nError: CL_INVALID_EVENT\n");
        break;
    case CL_INVALID_OPERATION:
        fprintf(stderr, "\nError: CL_INVALID_OPERATION\n");
        break;
    case CL_INVALID_GL_OBJECT:
        fprintf(stderr, "\nError: CL_INVALID_GL_OBJECT\n");
        break;
    case CL_INVALID_BUFFER_SIZE:
        fprintf(stderr, "\nError: CL_INVALID_BUFFER_SIZE\n");
        break;
    case CL_INVALID_MIP_LEVEL:
        fprintf(stderr, "\nError: CL_INVALID_MIP_LEVEL\n");
        break;
    case CL_INVALID_GLOBAL_WORK_SIZE:
        fprintf(stderr, "\nError: CL_INVALID_GLOBAL_WORK_SIZE\n");
        break;
#ifdef CL_VERSION_1_1
    case CL_INVALID_PROPERTY:
        fprintf(stderr, "\nError: CL_INVALID_PROPERTY\n");
        break;
#endif
#ifdef CL_VERSION_1_2
    case CL_INVALID_IMAGE_DESCRIPTOR:
        fprintf(stderr, "\nError: CL_INVALID_IMAGE_DESCRIPTOR\n");
        break;
    case CL_INVALID_COMPILER_OPTIONS:
        fprintf(stderr, "\nError: CL_INVALID_COMPILER_OPTIONS\n");
        break;
    case CL_INVALID_LINKER_OPTIONS:
        fprintf(stderr, "\nError: CL_INVALID_LINKER_OPTIONS\n");
        break;
    case CL_INVALID_DEVICE_PARTITION_COUNT:
        fprintf(stderr, "\nError: CL_INVALID_DEVICE_PARTITION_COUNT\n");
        break;
#endif
#ifdef CL_VERSION_2_0
    case CL_INVALID_PIPE_SIZE:
        fprintf(stderr, "\nError: CL_INVALID_PIPE_SIZE\n");
        break;
    case CL_INVALID_DEVICE_QUEUE:
        fprintf(stderr, "\nError: CL_INVALID_DEVICE_QUEUE\n");
        break;
#endif
#ifdef CL_VERSION_2_2
    case CL_INVALID_SPEC_ID:
        fprintf(stderr, "\nError: CL_INVALID_SPEC_ID\n");
        break;
    case CL_MAX_SIZE_RESTRICTION_EXCEEDED:
        fprintf(stderr, "\nError: CL_MAX_SIZE_RESTRICTION_EXCEEDED\n");
        break;
#endif
        // SDK errors
    case CL_UTIL_INDEX_OUT_OF_RANGE:
        fprintf(stderr, "\nError: CL_UTIL_INDEX_OUT_OF_RANGE\n");
        break;
    case CL_UTIL_DEVICE_NOT_INTEROPERABLE:
        fprintf(stderr, "\nError: CL_UTIL_DEVICE_NOT_INTEROPERABLE\n");
        break;
    case CL_UTIL_FILE_OPERATION_ERROR:
        fprintf(stderr, "\nError: CL_UTIL_FILE_OPERATION_ERROR\n");
        break;
        // end of SDK errors
    default: fprintf(stderr, "\nUnknown error: %i\n", error); break;
    }
}
#endif // STB_ISH_UTILS_IMPLEMENTATION
