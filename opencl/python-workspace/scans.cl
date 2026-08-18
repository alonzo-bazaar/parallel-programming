__kernel void ks_block_scan(__global uint* global_data,
                            const uint global_data_size,
                            __local   uint* local_data,
                            const uint local_data_size) {
    const uint global_idx = get_global_id(0);
    const uint local_idx  = get_local_id(0);
    const bool is_active  = (global_idx < global_data_size);

    local_data[local_idx]=is_active?global_data[global_idx]:0;
    barrier(CLK_LOCAL_MEM_FENCE);

    uint tmp;
    for(uint stride = 1; stride < local_data_size; stride*=2) {
        if (local_idx >= stride) tmp = local_data[local_idx - stride];
        barrier(CLK_LOCAL_MEM_FENCE);
        if (local_idx >= stride) local_data[local_idx] += tmp;
        barrier(CLK_LOCAL_MEM_FENCE);

        // you can test for yourself that translitterating the textbook to opencl
        // doesn't work, comment the 4 lines above and uncomment these two below
        // it will break
        // if (local_idx >= stride)
        //     local_data[local_idx] += local_data[local_idx - stride];
    }

    if(is_active) global_data[global_idx] = local_data[local_idx];
}

// expected to be launched with only one work group
// this kernel, together with the above and below kernels, provides *one* level
// of heirarchical scan
__kernel void ks_last_elt_scan(__global uint* global_data,
                               const uint global_data_size,

                               __local uint* chunk_sum_scan,
                               const uint number_of_chunks,
                               const uint single_chunk_size) {
    const uint local_idx = get_local_id(0);

    // given how last step was plus scanning all chunks individually, here  the
    // last element in any given chunk is the sum of all elements in that chunk
    // so let's get the index where that sum is housed in the `global_data` array
    const uint chunk_sum_global_idx =
        (single_chunk_size * local_idx) +
        (single_chunk_size - 1);

    // is the above index/the index this work item is responsible for valid?
    // that is, is the index in bounds?
    // that is, should  this work item do shit or is it an excess work item
    // spawned for alignment's sake?
    const bool is_active = (chunk_sum_global_idx < global_data_size);

    // and now, the same logic as above, with different names
    chunk_sum_scan[local_idx]=is_active?global_data[chunk_sum_global_idx]:0;
    barrier(CLK_LOCAL_MEM_FENCE);

    uint tmp;
    for(uint stride = 1; stride < number_of_chunks; stride*=2) {
        if (local_idx >= stride) tmp = chunk_sum_scan[local_idx - stride];
        barrier(CLK_LOCAL_MEM_FENCE);
        if (local_idx >= stride) chunk_sum_scan[local_idx] += tmp;
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if(is_active) global_data[chunk_sum_global_idx] = chunk_sum_scan[local_idx];
}

// filling pass has no algorithm specific logic to it
// we can use the same filling pass for all heirarchical scans we implement
__kernel void filling_pass(__global uint* global_data,
                           const uint global_data_size,
                           const uint single_chunk_size) {
    const uint global_idx = get_global_id(0);
    uint prev_chunk_end_idx = global_idx - (global_idx % single_chunk_size);
    prev_chunk_end_idx -= (prev_chunk_end_idx != 0);
    const uint curr_chunk_end_idx = prev_chunk_end_idx + single_chunk_size;
    const uint prev_chunk_end = global_data[prev_chunk_end_idx];
    const bool is_active = (global_idx < global_data_size);

    barrier(CLK_GLOBAL_MEM_FENCE);
    if(is_active && prev_chunk_end_idx && (global_idx != curr_chunk_end_idx))
        global_data[global_idx] += prev_chunk_end;
}



// below should probably go some #define'd bullshit to have the kernels above but
// coarsened to have every thread to a prefixed bunch of work 
// I want a generic, say
// #define ks_cbs_def(n) __kernel void ks_coarse_block_scan_ ## n () { /* .. */ }
// and then implement that for all desired thread coarsening options as
// ks_cbs_def( 1) // for benchmarking purposes
// ks_cbs_def( 2)
// ks_cbs_def( 4)
// ks_cbs_def( 8)
// ks_cbs_def(16)
// ks_cbs_def(32)
// ks_cbs_def(64)

// the above should most likely be done after I implement brent kung as well as kogge stone 
// make definition macros both for coarsened kogge stone and  for coarsened brent kung

// (maybe parameterize on op as well? tho that seems more like a codegen thing :/)
// (should probably fuck around a bit with shsl to make it more useable for codegen)

uint largest_power_of_2_less_than(uint i) {
    i |= (i>> 1);
    i |= (i>> 2);
    i |= (i>> 4);
    i |= (i>> 8);
    i |= (i>>16);
    return i & ~(i>>1);
}

__kernel void bk_block_scan(__global uint* global_data,
                            const uint global_data_size,
                            __local   uint* local_data,
                            const uint local_data_size) {
    const uint global_idx = get_global_id(0);
    const uint local_idx  = get_local_id(0);
    const bool is_active  = (global_idx < global_data_size);
    const uint section_size = largest_power_of_2_less_than(local_data_size);

    local_data[local_idx]=is_active?global_data[global_idx]:0;
    barrier(CLK_LOCAL_MEM_FENCE);

    // can't have data[i] += data[j] since that's a race condition somehow
    // so we do tmp = data[j]; barrier(); data[i] += tmp

    for(uint stride = 1; stride < local_data_size; stride*=2) {
        const uint curr_iteration_index =
            (local_idx+1) * 2*stride - 1;
        const bool is_active_on_iteration =
            curr_iteration_index < local_data_size;

        const uint tmp =
            is_active_on_iteration
            ?local_data[curr_iteration_index - stride]
            :0;
        barrier(CLK_LOCAL_MEM_FENCE);

        // for some reason it needs this if even if adds 0 (noop) when
        // curr_iteration_index >= local_data_size
        // it randomly breaks without this and I don't got a damn clue as to why
        // thanks clanker for finding this tho I still have no idea what
        // caused it :/
        if(is_active_on_iteration)
            local_data[curr_iteration_index] += tmp;
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    // second pass, distribute partial sums
    for(uint stride = section_size; stride; stride>>=1) {
        const uint curr_iteration_index =
            (local_idx+1) * (stride*2) - 1 + stride;
        const bool is_active_on_iteration =
            curr_iteration_index < local_data_size;

        const uint tmp =
            is_active_on_iteration
            ?local_data[curr_iteration_index - stride]
            :0;
        barrier(CLK_LOCAL_MEM_FENCE);

        if(is_active_on_iteration)
            local_data[curr_iteration_index] += tmp;
        barrier(CLK_LOCAL_MEM_FENCE);
    }


    if(is_active) global_data[global_idx] = local_data[local_idx];
}

__kernel void bk_last_elt_scan(__global uint* global_data,
                               const uint global_data_size,
                               __local uint* chunk_sum_scan,
                               const uint number_of_chunks,
                               const uint single_chunk_size) {
    const uint local_idx = get_local_id(0);
    const uint chunk_sum_global_idx =
        (single_chunk_size * local_idx) +
        (single_chunk_size - 1);
    const bool is_active = (chunk_sum_global_idx < global_data_size);
    const uint section_size = largest_power_of_2_less_than(number_of_chunks);

    chunk_sum_scan[local_idx]=is_active?global_data[chunk_sum_global_idx]:0;
    barrier(CLK_LOCAL_MEM_FENCE);

    for(uint stride = 1; stride < number_of_chunks; stride*=2) {
        const uint curr_iteration_index =
            (local_idx+1) * 2*stride - 1;
        const bool is_active_on_iteration =
            curr_iteration_index < number_of_chunks;

        const uint tmp =
            is_active_on_iteration
            ?chunk_sum_scan[curr_iteration_index - stride]
            :0;
        barrier(CLK_LOCAL_MEM_FENCE);

        if(is_active_on_iteration)
            chunk_sum_scan[curr_iteration_index] += tmp;
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    for(uint stride = section_size; stride; stride>>=1) {
        const uint curr_iteration_index =
            (local_idx+1) * (stride*2) - 1 + stride;
        const bool is_active_on_iteration =
            curr_iteration_index < number_of_chunks;
        
        const uint tmp =
            is_active_on_iteration
            ?chunk_sum_scan[curr_iteration_index - stride]
            :0;
        barrier(CLK_LOCAL_MEM_FENCE);

        if(is_active_on_iteration)
            chunk_sum_scan[curr_iteration_index] += tmp;
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if(is_active) global_data[chunk_sum_global_idx] = chunk_sum_scan[local_idx];
}
