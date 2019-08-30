#version 440

layout(location = 0) in vec4 v;
layout(location = 0) out vec2 pos;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    mat4 rotation;
    vec4 color;
    float pattern;
    int projection;
} ubuf;

out gl_PerVertex { vec4 gl_Position; float gl_PointSize; };

void main()
{
    vec4 p = ubuf.matrix * v;

    if (ubuf.projection != 0) {
        vec4 proj = ubuf.rotation * p;
        gl_Position = vec4(proj.x, proj.y, 0, proj.z);
    } else {
        gl_Position = p;
    }

    pos = v.xy * 1.37;

    gl_PointSize = 1.0;
}
