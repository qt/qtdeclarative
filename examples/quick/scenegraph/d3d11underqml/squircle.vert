static float4 gl_Position;
static float4 vertices;
static float2 coords;

struct SPIRV_Cross_Input
{
    float4 vertices : TEXCOORD0;
};

struct SPIRV_Cross_Output
{
    float2 coords : TEXCOORD0;
    float4 gl_Position : SV_Position;
};

void vert_main()
{
    gl_Position = vertices;
    coords = vertices.xy;
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    vertices = stage_input.vertices;
    vert_main();
    SPIRV_Cross_Output stage_output;
    stage_output.gl_Position = gl_Position;
    stage_output.coords = coords;
    return stage_output;
}
