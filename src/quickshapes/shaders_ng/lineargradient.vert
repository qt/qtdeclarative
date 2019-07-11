#version 440

layout(location = 0) in vec4 vertexCoord;
layout(location = 1) in vec4 vertexColor;

layout(location = 0) out float gradTabIndex;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec2 gradStart;
    vec2 gradEnd;
    float opacity;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    vec2 gradVec = ubuf.gradEnd - ubuf.gradStart;
    gradTabIndex = dot(gradVec, vertexCoord.xy - ubuf.gradStart) / (gradVec.x * gradVec.x + gradVec.y * gradVec.y);
    gl_Position = ubuf.matrix * vertexCoord;
}
