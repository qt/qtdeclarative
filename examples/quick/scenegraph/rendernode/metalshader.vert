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
    float4 v_color [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float4 pos [[attribute(0)]];
    float4 color [[attribute(1)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant buf& ubuf [[buffer(0)]])
{
    main0_out out = {};
    out.v_color = in.color;
    out.gl_Position = ubuf.matrix * in.pos;
    return out;
}

