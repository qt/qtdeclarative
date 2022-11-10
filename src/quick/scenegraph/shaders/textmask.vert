uniform highp mat4 modelViewMatrix;
uniform highp mat4 projectionMatrix;
uniform highp vec2 textureScale;
uniform highp float dpr;

attribute highp vec4 vCoord;
attribute highp vec2 tCoord;

varying highp vec2 sampleCoord;

void main()
{
     sampleCoord = tCoord * textureScale;
     vec4 xformed = modelViewMatrix * vCoord;
     gl_Position = projectionMatrix * vec4(floor(xformed.xyz * dpr + 0.5) / dpr, xformed.w);
}
