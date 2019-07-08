#version 440

layout(location = 0) in vec2 qt_TexCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D qt_Texture;

void main()
{
    fragColor = texture(qt_Texture, qt_TexCoord);
}
