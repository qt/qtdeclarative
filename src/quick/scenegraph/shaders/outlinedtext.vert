uniform highp mat4 modelViewMatrix;
uniform highp mat4 projectionMatrix;
uniform highp vec2 textureScale;
uniform highp vec2 shift;
uniform highp float dpr;

attribute highp vec4 vCoord;
attribute highp vec2 tCoord;

varying highp vec2 sampleCoord;
varying highp vec2 sCoordUp;
varying highp vec2 sCoordDown;
varying highp vec2 sCoordLeft;
varying highp vec2 sCoordRight;

void main()
{
     sampleCoord = tCoord * textureScale;
     sCoordUp = (tCoord - vec2(0.0, -1.0)) * textureScale;
     sCoordDown = (tCoord - vec2(0.0, 1.0)) * textureScale;
     sCoordLeft = (tCoord - vec2(-1.0, 0.0)) * textureScale;
     sCoordRight = (tCoord - vec2(1.0, 0.0)) * textureScale;
     vec4 xformed = modelViewMatrix * vCoord;
     gl_Position = projectionMatrix * vec4(floor(xformed.xyz * dpr + 0.5) / dpr, xformed.w);
}
