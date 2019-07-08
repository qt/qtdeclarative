#version 440

layout(location = 0) in vec4 vertexCoord;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec4 color;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    gl_Position = ubuf.matrix * vertexCoord;
}
