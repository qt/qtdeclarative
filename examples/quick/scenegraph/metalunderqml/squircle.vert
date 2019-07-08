#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct main0_out
{
    float2 coords [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float4 vertices [[attribute(0)]];
};

vertex main0_out main0(main0_in in [[stage_in]])
{
    main0_out out = {};
    out.gl_Position = in.vertices;
    out.coords = in.vertices.xy;
    return out;
}
