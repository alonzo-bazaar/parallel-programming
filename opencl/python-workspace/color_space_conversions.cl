// https://en.wikipedia.org/wiki/HSL_and_HSV
// 
// NOTE: we wanna represent an hsv pixel as a uchar3 (or uchar4) as well
// this means we'll have to remap the usual hsv value ranges
// (or at least the ones used in the wikipedia page)
// hue:        [0-360] -> [0-255]
// saturation: [0-1]   -> [0-255]
// value:      [0-1]   -> [0-255]

#define max2(a, b) (a>b?a:b)
#define max3(a, b, c) (a>b?max2(a,c):max2(b,c))

#define min2(a, b) (a<b?a:b)
#define min3(a, b, c) (a<b?min2(a,c):min2(b,c))

// the wikipedia article uses this one magnitude a lot
#define HUE_SLICE_SIZE ((uchar)(UCHAR_MAX/6))

// to divide two 0-1 values represented as 0-255
// into a third 0-1 value represented as 0-255
// so, for instance divide the r channel by the g channel and get a value you can
// put into another image channel
// no clue what the order of operations is so fully parenthesize everything
#define CH_DIV(a, b)  ( (uchar)(( (((ushort)a) * 256) / b )) )
#define CH_MUL(a, b)  ( (uchar)(( (((ushort)a) * b) / 256 )) )

// https://en.wikipedia.org/wiki/HSL_and_HSV#Color_conversion_formulae
// for a single pixel first
uchar3 to_hsv(const uchar r, const uchar g, const uchar b) {
    // https://en.wikipedia.org/wiki/HSL_and_HSV#From_RGB
    // https://en.wikipedia.org/wiki/HSL_and_HSV#General_approach
    const uchar x_max = max3(r,g,b);
    const uchar x_min = min3(r,g,b);

    // chroma (ie: range)
    const uchar c = x_max - x_min;
    // value
    const uchar v = x_max;
    // saturation
    const uchar s = v==0?0:CH_DIV(c,v);
    // hue
    const uchar h = (c==0?0
                       // which one was the max? r, g, or b?
                       :v==r?    (((((short)HUE_SLICE_SIZE*((short)g-b)/c)) + 6) % 6)
                       :v==g?      (((short)HUE_SLICE_SIZE*((short)b-r)/c)  + 2)
                       :/*v==b?*/  (((short)HUE_SLICE_SIZE*((short)r-g)/c)  + 4));
    return (uchar3){h, s, v};
}


// https://en.wikipedia.org/wiki/HSL_and_HSV#HSV_to_RGB_alternative
// k   : {0, 1, 2, 3, 4,  5}
// 4-k : {4, 3, 2, 1, 0, -1}
// 1   : {1, 1, 1, 1, 1,  1}
// 0   : {0, 0, 0, 0, 0,  0}
// max(0, min(k, 4-k, 1)) =
__constant static const uchar karr[6] = {0, 1, 1, 1, 0, 1};
uchar to_rgb_f(const uchar h, const char s, const uchar v, const uchar n) {
    const uchar i = ((n + (h/HUE_SLICE_SIZE))+6) %6;
    const uchar k = karr[i];
    return v - (k * CH_MUL(v, s));
}

uchar3 to_rgb(const uchar h, const uchar s, const uchar v) {
    return (uchar3) {
        to_rgb_f(h, s, v, 5),
        to_rgb_f(h, s, v, 3),
        to_rgb_f(h, s, v, 1),
    };
}

/*
uchar4 to_rgba(const uchar h, const uchar s, const uchar v, const uchar a) {
    const uchar3 rgb = to_rgb(h, s, v);
    return (uchar4){rgb.x, rgb.y, rgb.z, a};
}
uchar4 to_hsva(const uchar r, const uchar g, const uchar b, const uchar a) {
    const uchar3 hsv = to_hsv(r, g, b);
    return (uchar4){hsv.x, hsv.y, hsv.z, a};
}
*/

// kernels to convert a whole image at once
__kernel void rgb2hsv(const __global uchar* rgb_img,
                      __global uchar* hsv_img,
                      const uint n_pixels) {
    const uint i = get_global_id(0);
    if(i<n_pixels) {
        const __global uchar*rgb_ptr = rgb_img + (i*3);
        __global uchar*hsv_ptr = hsv_img + (i*3);
        const uchar3 hsv = to_hsv(rgb_ptr[0], rgb_ptr[1], rgb_ptr[2]);
        hsv_ptr[0]=hsv.x;
        hsv_ptr[1]=hsv.y;
        hsv_ptr[2]=hsv.z;
    }
}

__kernel void hsv2rgb(const __global uchar* hsv_img,
                      __global uchar* rgb_img,
                      const uint n_pixels) {
    const uint i = get_global_id(0);
    if(i<n_pixels) {
        const __global uchar*hsv_ptr = hsv_img + (i*3);
        __global uchar*rgb_ptr = rgb_img + (i*3);
        const uchar3 rgb = to_rgb(hsv_ptr[0], hsv_ptr[1], hsv_ptr[2]);
        rgb_ptr[0]=rgb.x;
        rgb_ptr[1]=rgb.y;
        rgb_ptr[2]=rgb.z;
    }
}
