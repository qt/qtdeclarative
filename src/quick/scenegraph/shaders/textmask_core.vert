#version 150 core

in vec4 vCoord;
in vec2 tCoord;

out vec2 sampleCoord;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform vec2 textureScale;
uniform float dpr;

void main()
{
     sampleCoord = tCoord * textureScale;
     vec4 xformed = modelViewMatrix * vCoord;
     gl_Position = projectionMatrix * vec4(floor(xformed.xyz * dpr + 0.5) / dpr, xformed.w);
}
