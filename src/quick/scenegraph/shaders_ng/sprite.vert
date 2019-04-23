#version 440

layout(location = 0) in vec2 vPos;
layout(location = 1) in vec2 vTex;

layout(location = 0) out vec4 fTexS;
layout(location = 1) out float progress;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec4 animPos; // x,y, x,y (two frames for interpolation)
    vec3 animData; // w,h(premultiplied of anim), interpolation progress
    float opacity;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    progress = ubuf.animData.z;

    // Calculate frame location in texture
    fTexS.xy = ubuf.animPos.xy + vTex.xy * ubuf.animData.xy;

    // Next frame is also passed, for interpolation
    fTexS.zw = ubuf.animPos.zw + vTex.xy * ubuf.animData.xy;

    gl_Position = ubuf.matrix * vec4(vPos.x, vPos.y, 0, 1);
}
