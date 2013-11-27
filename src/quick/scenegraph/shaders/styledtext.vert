uniform highp mat4 matrix;
uniform highp vec2 textureScale;
uniform highp vec2 shift;

attribute highp vec4 vCoord;
attribute highp vec2 tCoord;

varying highp vec2 sampleCoord;
varying highp vec2 shiftedSampleCoord;

void main()
{
     sampleCoord = tCoord * textureScale;
     shiftedSampleCoord = (tCoord - shift) * textureScale;
     gl_Position = matrix * vCoord;
}