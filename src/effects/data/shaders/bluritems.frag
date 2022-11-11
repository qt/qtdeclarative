#version 440

layout(location = 0) in vec2 texCoord0;
layout(location = 1) in vec2 texCoord1;
layout(location = 2) in vec2 texCoord2;
layout(location = 3) in vec2 texCoord3;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 offset;
};

layout(binding = 1) uniform sampler2D source;

void main() {
    vec4 sourceColor = (texture(source, texCoord0) + texture(source, texCoord1) +
                        texture(source, texCoord2) + texture(source, texCoord3)) / 4.0;
    fragColor = sourceColor * qt_Opacity;
}
