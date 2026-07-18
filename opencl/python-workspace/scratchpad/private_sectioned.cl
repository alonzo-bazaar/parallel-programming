// host is expected to have zero'd out outhist before calling this kernel
__kernel void sectioned_hist(const __global uchar* input,
                   __global uint* histbuf, // 32?
                   const uint input_size,
                   const uint hist_size) {
    const uint i = get_global_id(0);
    if(i < hist_size)
        histbuf[i] = 0;
    barrier(CLK_GLOBAL_MEM_FENCE);

    // assign a block of indices to each thread
    const uint block_size = ceil(input_size / (float)(get_global_size(0)));
    // const uint block_size = input_size / (get_global_size(0) - 1);
    const uint block_start = block_size * i;
    const uint block_end = min(block_start+block_size, input_size);

    for(uint j = block_start; j<block_end; j++)
        atomic_add(&histbuf[input[j]], 1);
}
