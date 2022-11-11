#version 440

layout(location = 0) in vec4 qt_Vertex;
layout(location = 1) in vec2 qt_MultiTexCoord0;
layout(location = 0) out vec2 texCoord0;
layout(location = 1) out vec2 texCoord1;
layout(location = 2) out vec2 texCoord2;
layout(location = 3) out vec2 texCoord3;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 offset;
};

void main() {
    float dither = 0.33;
    texCoord0 = vec2(qt_MultiTexCoord0.x + offset.x, qt_MultiTexCoord0.y + offset.y * dither);
    texCoord1 = vec2(qt_MultiTexCoord0.x + offset.x * dither, qt_MultiTexCoord0.y - offset.y);
    texCoord2 = vec2(qt_MultiTexCoord0.x - offset.x * dither, qt_MultiTexCoord0.y + offset.y);
    texCoord3 = vec2(qt_MultiTexCoord0.x - offset.x, qt_MultiTexCoord0.y - offset.y * dither);
    gl_Position = qt_Matrix * qt_Vertex;
}
