__kernel void vec_add(__global float* out,
                      __global const float* in1,
                      __global const float* in2) {
    size_t i = get_global_id(0);
    printf("PUTTANACCIA DI QUELLA %zu\n", i);
    out[i] = in1[i] + in2[i];
}
