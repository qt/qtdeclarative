#version 440

layout(location = 0) in vec4 qt_Vertex;
layout(location = 1) in vec2 qt_MultiTexCoord0;

layout(location = 0) out vec2 qt_TexCoord0;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec4 subRect; // in pixels
};

// Sampled in the vertex shader as well as in the fragment shader:
layout(binding = 1) uniform sampler2D source;

void main() {
    vec2 size = vec2(textureSize(source, 0));
    qt_TexCoord0 = (subRect.xy + subRect.zw * qt_MultiTexCoord0) / size;
    gl_Position = qt_Matrix * qt_Vertex;
}
