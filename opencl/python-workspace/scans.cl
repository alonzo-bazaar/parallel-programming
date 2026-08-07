// -*- mode:c -*-

// just one thread, quite horrible, but a baseline
__kernel void ungodly(__global int* data, uint data_size) {
    if(get_global_id(0)==0) {
        for(uint i = 1; i<data_size; ++i) {
            data[i] += data[i-1];
        }
    }
}

// the "active" think suggested by clanker
// thank you clanker, I suppose
// cause of bug I had:
//     all kernels in a work group must execute the barrier so if I have an early
//     in one of the work elemenents and not in another then oopsie daisy, the
//     barrier is never satisified and the kernel hangs and you go shit yourself
//     and die
__kernel void kogge_stone_block_scan(__global uint* global_data,
                                     const uint global_data_size,
                                     __local uint* local_data,
                                     const uint local_data_size) {
    const uint gi = get_global_id(0);
    const uint li = get_local_id(0);
    const bool active = gi < global_data_size;

    if(active) local_data[li] = global_data[gi];
    
    barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);
    for(uint stride = 1; stride < local_data_size; stride*=2) {
        if(active && li >= stride) local_data[li] += local_data[li-stride];
        barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);
    }
    barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);

    if(active) global_data[gi] = local_data[li];
}

// ci si aspetta che venga lanciato con un solo work group questo
// (e che quindi get_global_id(0) == get_local_id(0))
__kernel void kogge_stone_last_elem_scan(__global uint* global_data,
                                         const uint global_data_size,
                                         __local uint* local_data,
                                         const uint local_data_size,
                                         const uint chunk_size) {
    // index into local data (index of chunk we're working on)
    const uint li = get_local_id(0);
    const uint chunk_end = (li+1)*chunk_size-1;
    const bool active = chunk_end < global_data_size;

    if(active) local_data[li] = global_data[chunk_end];

    // stessa logica di sopra
    for(uint stride = 1; stride < local_data_size; stride*=2) {
        barrier(CLK_LOCAL_MEM_FENCE);
        if(active && li >= stride) local_data[li] += local_data[li-stride];
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    if(active) global_data[chunk_end] = local_data[li];
}

__kernel void kogge_stone_filling_pass(__global uint* data,
                                       const uint data_size,
                                       const uint chunk_size) {
    const uint data_index = get_global_id(0);
    const uint chunk_index = get_group_id(0);
    const uint prev_chunk_end_index = chunk_index * chunk_size - 1;
    const uint curr_chunk_end_index = (chunk_index + 1) * chunk_size - 1;
    const uint prev_chunk_end = data[prev_chunk_end_index];

    if((data_index < data_size) &&
       (data_index != curr_chunk_end_index) &&
       (chunk_index != 0))
        data[data_index] += prev_chunk_end;
}
