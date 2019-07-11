#version 440

layout(location = 0) in vec2 texCoord;
layout(location = 1) in float vertexOpacity;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D qt_Texture;

void main()
{
    fragColor = texture(qt_Texture, texCoord) * vertexOpacity;
}
