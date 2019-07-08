#version 440

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 vertexColor;
layout(location = 2) in vec4 vertexOffset;

layout(location = 0) out vec4 color;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec2 pixelSize;
    float opacity;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    vec4 pos = ubuf.matrix * vertex;
    gl_Position = pos;

    if (vertexOffset.x != 0.) {
        vec4 delta = ubuf.matrix[0] * vertexOffset.x;
        vec2 dir = delta.xy * pos.w - pos.xy * delta.w;
        vec2 ndir = .5 * ubuf.pixelSize * normalize(dir / ubuf.pixelSize);
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
        vec4 delta = ubuf.matrix[1] * vertexOffset.y;
        vec2 dir = delta.xy * pos.w - pos.xy * delta.w;
        vec2 ndir = .5 * ubuf.pixelSize * normalize(dir / ubuf.pixelSize);
        dir -= ndir * delta.w * pos.w;
        float numerator = dot(dir, ndir * pos.w * pos.w);
        float scale = 0.0;
        if (numerator < 0.0)
            scale = 1.0;
        else
            scale = min(1.0, numerator / dot(dir, dir));
        gl_Position += scale * delta;
    }

    color = vertexColor * ubuf.opacity;
}
