#version 440

layout(location = 0) in vec2 vPos;
layout(location = 1) in vec2 vTex;

layout(location = 0) out vec4 fTexS;
layout(location = 1) out float progress;

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 matrix[QSHADER_VIEW_COUNT];
#else
    mat4 matrix;
#endif
    vec4 animPos; // x,y, x,y (two frames for interpolation)
    vec3 animData; // w,h(premultiplied of anim), interpolation progress
    float opacity;
};

void main()
{
    progress = animData.z;

    // Calculate frame location in texture
    fTexS.xy = animPos.xy + vTex.xy * animData.xy;

    // Next frame is also passed, for interpolation
    fTexS.zw = animPos.zw + vTex.xy * animData.xy;

#if QSHADER_VIEW_COUNT >= 2
    gl_Position = matrix[gl_ViewIndex] * vec4(vPos.x, vPos.y, 0, 1);
#else
    gl_Position = matrix * vec4(vPos.x, vPos.y, 0, 1);
#endif
}
