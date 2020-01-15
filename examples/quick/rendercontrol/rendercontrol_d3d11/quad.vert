struct VSIn
{
    uint id : SV_VertexId;
};

struct VSOut
{
    float2 coord : TEXCOORD0;
    float4 pos : SV_Position;
};

static const float2 quadPos[6] = {
    float2(-0.5, 0.5),
    float2(0.5, -0.5),
    float2(-0.5, -0.5),
    float2(0.5, 0.5),
    float2(0.5, -0.5),
    float2(-0.5, 0.5)
};

static const float2 quadUv[6] = {
    float2(0.0, 0.0),
    float2(1.0, 1.0),
    float2(0.0, 1.0),
    float2(1.0, 0.0),
    float2(1.0, 1.0),
    float2(0.0, 0.0),
};

VSOut quad_vs_main(VSIn input)
{
    VSOut output;
    output.pos = float4(quadPos[input.id].xy, 0.0, 1.0);
    output.coord = quadUv[input.id];
    return output;
}
