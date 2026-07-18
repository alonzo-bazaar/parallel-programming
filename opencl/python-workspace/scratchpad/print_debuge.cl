__kernel void debuge(const __global uchar* inbuf,
                     __global uint* histbuf,
                     const uint input_size,
                     const uint hist_size) {
    const uint i = get_global_id(0);
    if(i==0) {
        printf("[HAIL MARY] ");
        for(uint j = 0; j<hist_size; j++) {
            printf("%u, ", histbuf[j]);
        }
        printf("\n");
    }
}
