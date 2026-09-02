// must be launched with at least max(hist_size, input_size) threads
// we also assume local dim is >= hist_size for now

// local buffers must either be of constant length (ie __local uint buf[256])
// or be passed to the kernel by the caller as local memory objects
// which is what we're gonna do here because idk, seems more general
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

// we have an image with num_chans channels
// and we want to compute the histogram of its chan_idx'th channel
// there's a lot of code repetition here but phrasing this in terms of a more
// generic kernel we pass parameters too would be rather wasteful, we'd have a
// lot of local indices in vram that could have been hardcoded and become part
// of gpu code without needing to occupy one of our precious registers
// 
// opencl c doesn't have templates so I'm using c style macros instead to do
// "parametric hardcoding"
#define def_channel_hist(name, num_chans, chan_idx)                     \
    __kernel void name(const __global uchar* input_buf,                 \
                       __global uint* global_hist,                      \
                       __local uint* local_hist,                        \
                       const uint input_npixels,                        \
                       const uint hist_size) {                          \
        const uint gi = get_global_id(0);                               \
        const uint li = get_local_id(0);                                \
                                                                        \
        if(li<hist_size) local_hist[li] = 0;                            \
                                                                        \
        barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);            \
                                                                        \
        if(gi<input_npixels)                                            \
            atomic_add(&local_hist[input_buf[(gi * num_chans) + chan_idx]], 1); \
                                                                        \
        barrier(CLK_GLOBAL_MEM_FENCE | CLK_LOCAL_MEM_FENCE);            \
        if(li < hist_size)                                              \
            atomic_add(&global_hist[li], local_hist[li]);               \
    }

def_channel_hist(xyz_x_hist, 3, 0)
def_channel_hist(xyz_y_hist, 3, 1)
def_channel_hist(xyz_z_hist, 3, 2)

def_channel_hist(xyzw_x_hist, 4, 0)
def_channel_hist(xyzw_y_hist, 4, 1)
def_channel_hist(xyzw_z_hist, 4, 2)
def_channel_hist(xyzw_w_hist, 4, 3)
