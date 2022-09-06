#version 440

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec3 barycentric;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
} ubuf;

void main()
{
    float f = min(barycentric.x, min(barycentric.y, barycentric.z));
    float d = fwidth(f * 1.5);
    float alpha = smoothstep(0.0, d, f);

    //alpha = 1.0 - step(0.5, barycentric.x);
    fragColor = vec4(1.0, 0.2, 1.0, 1.0) * (1.0 - alpha);
}
