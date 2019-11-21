uniform highp mat4 matrix;
uniform highp vec2 textureScale;
uniform highp vec2 shift;
uniform highp float dpr;

attribute highp vec4 vCoord;
attribute highp vec2 tCoord;

varying highp vec2 sampleCoord;
varying highp vec2 shiftedSampleCoord;

void main()
{
     sampleCoord = tCoord * textureScale;
     shiftedSampleCoord = (tCoord - shift) * textureScale;
     vec3 dprSnapPos = floor(vCoord.xyz * dpr + 0.5) / dpr;
     gl_Position = matrix * vec4(dprSnapPos, vCoord.w);
}
