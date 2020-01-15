struct PSIn
{
    float2 coord : TEXCOORD0;
};

struct PSOut
{
    float4 color : SV_Target;
};

Texture2D tex : register(t0);
SamplerState samp : register(s0);

PSOut quad_ps_main(PSIn input)
{
    PSOut output;
    output.color = tex.Sample(samp, input.coord);
    return output;
}
