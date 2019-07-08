#version 440

layout(location = 0) in vec4 vCoord;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    gl_Position = ubuf.matrix * vCoord;
}
