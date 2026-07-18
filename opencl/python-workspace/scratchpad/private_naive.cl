// must be launched with at least max(hist_size, input_size) threads
// we also assume local dim is >= hist_size for now

// local buffers must either be of constant length (ie __local uint buf[256])
// or be passed to the kernel by the caller as local memory objects
// which is what we're gonna do here because idk, seems more general
__kernel void private_naive(const __global uchar* input,
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
