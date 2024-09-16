#version 440

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 multiTexCoord;
layout(location = 2) in vec2 vertexOffset;
layout(location = 3) in vec2 texCoordOffset;

layout(location = 0) out vec2 texCoord;
layout(location = 1) out float vertexOpacity;

layout(std140, binding = 0) uniform buf {
#if QSHADER_VIEW_COUNT >= 2
    mat4 qt_Matrix[QSHADER_VIEW_COUNT];
#else
    mat4 qt_Matrix;
#endif
    float opacity;
    vec2 pixelSize;
};

void main()
{
#if QSHADER_VIEW_COUNT >= 2
    vec4 pos = qt_Matrix[gl_ViewIndex] * vertex;
    vec4 m0 = qt_Matrix[gl_ViewIndex][0];
    vec4 m1 = qt_Matrix[gl_ViewIndex][1];
#else
    vec4 pos = qt_Matrix * vertex;
    vec4 m0 = qt_Matrix[0];
    vec4 m1 = qt_Matrix[1];
#endif
    gl_Position = pos;
    texCoord = multiTexCoord;

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
        texCoord.x += scale * texCoordOffset.x;
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
        texCoord.y += scale * texCoordOffset.y;
    }

    bool onEdge = any(notEqual(vertexOffset, vec2(0.)));
    bool outerEdge = all(equal(texCoordOffset, vec2(0.)));
    if (onEdge && outerEdge)
        vertexOpacity = 0.;
    else
        vertexOpacity = opacity;
}
