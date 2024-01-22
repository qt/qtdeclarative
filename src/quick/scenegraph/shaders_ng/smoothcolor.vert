#version 440

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 vertexColor;
layout(location = 2) in vec4 vertexOffset;

layout(location = 0) out vec4 color;

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 matrix[QSHADER_VIEW_COUNT];
#else
    mat4 matrix;
#endif
    vec2 pixelSize;
    float opacity;
};

void main()
{
#if QSHADER_VIEW_COUNT >= 2
    vec4 pos = matrix[gl_ViewIndex] * vertex;
    vec4 m0 = matrix[gl_ViewIndex][0];
    vec4 m1 = matrix[gl_ViewIndex][1];
#else
    vec4 pos = matrix * vertex;
    vec4 m0 = matrix[0];
    vec4 m1 = matrix[1];
#endif
    gl_Position = pos;

    if (vertexOffset.x != 0.) {
        vec4 delta = m0 * vertexOffset.x;
        vec2 dir = delta.xy * pos.w - pos.xy * delta.w;
        vec2 ndir = .5 * pixelSize * normalize(dir / pixelSize);
        dir -= ndir * delta.w * pos.w;
        float numerator = dot(dir, ndir * pos.w * pos.w);
        float scale = 0.0;
        if (numerator < 0.0)
            scale = 1.0;
        else
            scale = min(1.0, numerator / dot(dir, dir));
        gl_Position += scale * delta;
    }

    if (vertexOffset.y != 0.) {
        vec4 delta = m1 * vertexOffset.y;
        vec2 dir = delta.xy * pos.w - pos.xy * delta.w;
        vec2 ndir = .5 * pixelSize * normalize(dir / pixelSize);
        dir -= ndir * delta.w * pos.w;
        float numerator = dot(dir, ndir * pos.w * pos.w);
        float scale = 0.0;
        if (numerator < 0.0)
            scale = 1.0;
        else
            scale = min(1.0, numerator / dot(dir, dir));
        gl_Position += scale * delta;
    }

    color = vertexColor * opacity;
}
