__kernel void add_into(__global float* in1,
                       __global float* in2,
                       __global float* out,
                       int size) {
    int i = get_global_id(0);
    if(i < size) {
        out[i] = in1[i] + in2[i];
    }
}
