#version 440

layout(location = 0) in vec4 aVertex;
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vShadeCoord;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    vec4 color;
    vec2 textureSize;
    float qt_Opacity;
};

out gl_PerVertex { vec4 gl_Position; };

void main() {
    gl_Position = qt_Matrix * aVertex;
    vTexCoord = aVertex.xy * textureSize;
    vShadeCoord = aTexCoord;
}
