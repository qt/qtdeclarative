#version 440

layout(location = 0) in vec4 vCoord;
layout(location = 1) in vec2 tCoord;

layout(location = 0) out vec2 sampleCoord;
layout(location = 1) out vec3 sampleFarLeft;
layout(location = 2) out vec3 sampleNearLeft;
layout(location = 3) out vec3 sampleNearRight;
layout(location = 4) out vec3 sampleFarRight;

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 matrix[QSHADER_VIEW_COUNT];
#else
    mat4 matrix;
#endif
    vec2 textureScale;
    vec4 color;
    float alphaMin;
    float alphaMax;
    // up to this point it must match distancefieldtext
    float fontScale;
    vec4 vecDelta;
};

void main()
{
    sampleCoord = tCoord * textureScale;
#if QSHADER_VIEW_COUNT >= 2
    gl_Position = matrix[gl_ViewIndex] * vCoord;
#else
    gl_Position = matrix * vCoord;
#endif

    // Calculate neighbor pixel position in item space.
    vec3 wDelta = gl_Position.w * vecDelta.xyw;
    vec3 farLeft = vCoord.xyw - 0.667 * wDelta;
    vec3 nearLeft = vCoord.xyw - 0.333 * wDelta;
    vec3 nearRight = vCoord.xyw + 0.333 * wDelta;
    vec3 farRight = vCoord.xyw + 0.667 * wDelta;

    // Calculate neighbor texture coordinate.
    vec2 scale = textureScale / fontScale;
    vec2 base = sampleCoord - scale * vCoord.xy;
    sampleFarLeft = vec3(base * farLeft.z + scale * farLeft.xy, farLeft.z);
    sampleNearLeft = vec3(base * nearLeft.z + scale * nearLeft.xy, nearLeft.z);
    sampleNearRight = vec3(base * nearRight.z + scale * nearRight.xy, nearRight.z);
    sampleFarRight = vec3(base * farRight.z + scale * farRight.xy, farRight.z);
}
