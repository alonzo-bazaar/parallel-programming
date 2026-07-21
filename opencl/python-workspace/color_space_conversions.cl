// https://en.wikipedia.org/wiki/HSL_and_HSL
// https://sqlpey.com/algorithm/hsl-rgb-conversion-implementations/#solution-4-java-implementation-float-precision-required
// 
// NOTE: we wanna represent an hsl pixel as a uchar3 (or uchar4) as well
// this means we'll have to remap the usual hsl value ranges
// (or at least the ones used in the wikipedia page)
// hue:        [0-360] <-> [0-255]
// saturation: [0-1]   <-> [0-255]
// value:      [0-1]   <-> [0-255]
#define MAX2(a, b) ((a>b)?a:b)
#define MIN2(a, b) ((a<b)?a:b)
#define MAX3(a, b, c) ((a>b)?MAX2(a,c):MAX2(b,c))
#define MIN3(a, b, c) ((a<b)?MIN2(a,c):MIN2(b,c))

uchar f2uchar(const float mag) {
    return (uchar)fmin(255, (mag * 256.0f));
}
float uchar2f(const uchar mag) {
    return ((float)mag) / 255.0f;
}

// for a single pixel first
// https://en.wikipedia.org/wiki/HSL_and_HSL#From_RGB
// https://en.wikipedia.org/wiki/HSL_and_HSL#General_approach
uchar3 rgb2hsl_pixel(const uchar r, const uchar g, const uchar b) {
    const uchar u_max = MAX3(r,g,b);
    const uchar u_min = MIN3(r,g,b);

    // r, g, and b as floats I can operate on
    const float rf = uchar2f(r);
    const float gf = uchar2f(g);
    const float bf = uchar2f(b);
    const float f_max = MAX3(rf, gf, bf);
    const float f_min = MIN3(rf, gf, bf);

    const float lf = (f_max+f_min)/2.0f;
    if(u_max == u_min)
        return (uchar3){0, 0, f2uchar(lf)};
    else {
        const float df = f_max - f_min;
        const float sf = (lf>0.5f) ?df/(2.0f-f_max-f_min) :df/(f_max+f_min);
        float hf;
        if(u_max == r) hf = (gf-bf) / df+(g<b?6.0f:0.0f);
        if(u_max == g) hf = (bf-rf) / df+2.0f;
        if(u_max == b) hf = (bf-rf) / df+2.0f;
        hf /= 6.0f;
        return (uchar3){f2uchar(hf), f2uchar(sf), f2uchar(lf)};
    }
}

float hue2rgb(float p, float q, float t) {
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f/2.0f) return q;
    if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
    return p;
}
uchar3 hsl2rgb_pixel(const uchar h, const uchar s, const uchar l) {
    const float hf = uchar2f(h);
    const float sf = uchar2f(s);
    const float lf = uchar2f(l);

    if(s == 0) {
        return (uchar3){l, l, l};
    }
    else {
        const float qf = lf<0.5f ?lf*(1+sf) :lf+sf-lf*sf;
        const float pf = 2*lf-qf;
        return (uchar3) {
            f2uchar(hue2rgb(pf, qf, hf + 1.0f/3.0f)),
            f2uchar(hue2rgb(pf, qf, hf)),
            f2uchar(hue2rgb(pf, qf, hf - 1.0f/3.0f))
        };
    }
}

// kernels to convert a whole image at once
__kernel void rgb2hsl(const __global uchar* rgb_img,
                      __global uchar* hsl_img,
                      const uint n_pixels) {
    const uint i = get_global_id(0);
    if(i<n_pixels) {
        const __global uchar*rgb_pxl = rgb_img + (i*3);
        __global uchar*hsl_pxl = hsl_img + (i*3);
        const uchar3 hsl = rgb2hsl_pixel(rgb_pxl[0], rgb_pxl[1], rgb_pxl[2]);
        hsl_pxl[0]=hsl.x;
        hsl_pxl[1]=hsl.y;
        hsl_pxl[2]=hsl.z;
    }
}

__kernel void hsl2rgb(const __global uchar* hsl_img,
                      __global uchar* rgb_img,
                      const uint n_pixels) {
    const uint i = get_global_id(0);
    if(i<n_pixels) {
        const __global uchar*hsl_pxl = hsl_img + (i*3);
        __global uchar*rgb_pxl = rgb_img + (i*3);
        const uchar3 rgb = hsl2rgb_pixel(hsl_pxl[0], hsl_pxl[1], hsl_pxl[2]);
        rgb_pxl[0]=rgb.x;
        rgb_pxl[1]=rgb.y;
        rgb_pxl[2]=rgb.z;
    }
}
