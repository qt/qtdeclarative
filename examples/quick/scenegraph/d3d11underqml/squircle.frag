cbuffer buf : register(b0)
{
    float ubuf_t : packoffset(c0);
};


static float2 coords;
static float4 fragColor;

struct SPIRV_Cross_Input
{
    float2 coords : TEXCOORD0;
};

struct SPIRV_Cross_Output
{
    float4 fragColor : SV_Target0;
};

void frag_main()
{
    float i = 1.0f - (pow(abs(coords.x), 4.0f) + pow(abs(coords.y), 4.0f));
    i = smoothstep(ubuf_t - 0.800000011920928955078125f, ubuf_t + 0.800000011920928955078125f, i);
    i = floor(i * 20.0f) / 20.0f;
    fragColor = float4((coords * 0.5f) + 0.5f.xx, i, i);
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    coords = stage_input.coords;
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.fragColor = fragColor;
    return stage_output;
}
