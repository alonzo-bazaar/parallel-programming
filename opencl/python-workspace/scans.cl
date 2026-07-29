// just one thread, quite horrible, but a baseline
__kernel void ungodly(__global int* data, uint data_size) {
    if(get_global_id(0)==0) {
        for(uint i = 1; i<data_size; ++i) {
            data[i] += data[i-1];
        }
    }
}

__kernel void kogge_stone_block_scan(__global uint* global_data, const uint data_size
                                     __local uint* local_data, const uint local_size) {
    const uint gi = get_global_id(0);
    const uint li = get_local_id(0);
    if(gi >= data_size)
        return;

    local_data[li] = global_data[gi];
    barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);
    
    for(uint stride = 1; stride < local_size; stride*=2) {
        barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);
        if(li >= stride)
            local_data[li] += local_data[li-stride];
    }

    globa_data[gi] = local_data[li];
}

// ci si aspetta che venga lanciato con un solo work group questo
// ( get_global_id(0) == get_local_id(0) )
__kernel void kogge_stone_first_elem_scan(__global uint* global_data,
                                          const uint global_data_size,
                                          const uint local_data_size) {
    const uint li = get_local_id(0);
    const uint gi = i*local_data_size;
    if(gi >= data_size)
        return;

    local_data[li] = global_data[gi];

    // stessa logica di sopra
    for(uint stride = 1; stride < local_size; stride*=2) {
        barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);
        if(li >= stride)
            local_data[li] += local_data[li-stride];
    }

    globa_data[gi] = local_data[li];
}

__kernel void kogge_stone_filling_scan(__global uint* global_data,
                                       const uint global_data_size,
                                       const uint local_data_size) {
    const uint gi = get_global_id(0);
    const uint li = get_local_id(0);

    if(gi >= global_data_size)
        return;

    const uint group_start_index = get_group_id(0) * local_data_size;
    const uint group_start = global_data[group_start_index];

    if(li != 0)
        global_data[gi] += group_start;
}
