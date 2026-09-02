// TODO: equalize y, z, and xyzw (macros)
// https://en.wikipedia.org/wiki/Histogram_equalization
// https://polaris000.medium.com/histogram-equalization-c67bfa9e2a3b
// HistEq ← { 𝕊img:
//     hist ← img ImgHist 256
//     hist_cdf ← +` hist
//     ⌊ 0.5+{(255× 𝕩⊏hist_cdf) ÷ ≠𝕩}⌾⥊ img
// }

#define def_tmin(t) t t ## _min(t a, t b) { return a>b?b:a; }
#define def_tmax(t) t t ## _max(t a, t b) { return a>b?a:b; }
def_tmin(uint)
def_tmax(uint)
def_tmin(float)
def_tmax(float)
void __kernel xyz_z_eqlz(__global uchar* xyz_arr,
                         __constant uint* histogram_cdf,
                         const uint xyz_len,
                         const uint hist_len) {
    const uint i = get_global_id(0);
    if(i > xyz_len) return;
    const uint xyz_i = (i * 3) + 2;
    const float val = float_min
        (255.0f,
         (float)histogram_cdf[xyz_arr[xyz_i]] * (float)hist_len / (float)xyz_len);
    xyz_arr[xyz_i] = (uchar)((uint)val);
}

// we only do one global memory access per buffer we're working with
// so it's probably not that advantageous to do corner turning here
// I think
// (also the gpu I'm running this on only supports work groups up to 256 elements
//  so to corner turn you'd need 256 accesses upfront which is already the max
//  amount of accesses you can expect of a gpu, may try corner turning just to see
//  if coalescing changes anything, but I think it's gonna improve worst case but
//  not average case)
