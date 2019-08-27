#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct buf
{
    float4x4 matrix;
    float opacity;
};

struct main0_out
{
    float4 fragColor [[color(0)]];
};

struct main0_in
{
    float4 v_color [[user(locn0)]];
};

fragment main0_out main0(main0_in in [[stage_in]], constant buf& ubuf [[buffer(0)]])
{
    main0_out out = {};
    out.fragColor = in.v_color * ubuf.opacity;
    return out;
}

