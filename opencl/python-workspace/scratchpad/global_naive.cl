// #define DEBUGGING

// must be launched with at least max(hist_size, input_size) threads
__kernel void global_naive(const __global uchar* inbuf,
                           __global uint* histbuf, // 32?
                           const uint input_size,
                           const uint hist_size) {
    // zero histogram out
    const uint i = get_global_id(0);
#ifdef DEBUGGING
    if(i == 0) {
        printf("[KERNEL PARAMETER] \"input size\",%u\n", input_size);
        printf("[KERNEL PARAMETER] \"histogram size\",%u\n", hist_size);
        printf("[KERNEL PARAMETER] \"input buffer\",%p\n", (__global void*)inbuf);
        printf("[KERNEL PARAMETER] \"histogram buffer\",%p\n", (__global void*)histbuf);
    }
#endif

    if(i<input_size) {
#ifdef DEBUGGING
        const uint picked = (uint)inbuf[i];
        printf("[PICKED INDEX SRC] %u,%u,=> %u\n", i, picked, histbuf[picked]);
        atomic_add(&histbuf[picked], 1);
        printf("[PICKED INDEX DST] %u,%u,=> %u\n", i, picked, histbuf[picked]);
#else
        const uint picked = (uint)inbuf[i];
        atomic_add(&histbuf[picked], 1);
#endif
    }
}
